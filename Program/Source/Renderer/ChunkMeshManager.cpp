#include "Renderer/ChunkMeshManager.h"

#include <iostream>
#include <cstring>

namespace {
	// Segregated size-class tiers, ordered smallest bucket first (MigrateToPool
	// relies on that ordering). A chunk borrows a slot from the SMALLEST bucket
	// that fits its quad count, so a flat 300-quad chunk never reserves a slot
	// sized for a 10k-quad mountain. Finer tiers => far less wasted bytes per
	// slot => we can afford many more slots for the same memory, which is what
	// keeps every visible chunk on screen instead of leaving holes.
	struct PoolTier { int bucketQuads; int slots; };
	constexpr PoolTier kTiers[] = {
		{   512,  512 },  //   7.00 MiB - flat plains / low-detail chunks
		{  1024,  768 },  //  21.00 MiB
		{  2048, 1024 },  //  56.00 MiB - the bulk of typical terrain lands here
		{  3584,  640 },  //  61.25 MiB
		{  6144,  320 },  //  52.50 MiB
		{ 12288,  128 },  //  42.00 MiB - rare dense mountain / cave chunks
	};
	constexpr int kTierCount = (int)(sizeof(kTiers) / sizeof(kTiers[0]));

	// Mirrors ChunkPool's internal layout: 16 bytes packed + 12 bytes index per quad.
	constexpr int kPackedBytesPerQuad = 16;
	constexpr int kIndexBytesPerQuad  = 12;
}

ChunkMeshManager::ChunkMeshManager() {
	// A live GL context is required: App constructs the Renderer (which creates
	// the context) before this manager, so we can allocate the arenas here.
	_pools.reserve(kTierCount);
	long long totalBytes = 0;
	int totalSlots = 0;
	for (const PoolTier& t : kTiers) {
		_pools.emplace_back(t.bucketQuads, t.slots);
		totalBytes += (long long)t.slots * t.bucketQuads * (kPackedBytesPerQuad + kIndexBytesPerQuad);
		totalSlots += t.slots;
	}
	std::cout << "[ChunkPool] allocated " << _pools.size() << " tiers, "
	          << totalSlots << " slots total, ~"
	          << (totalBytes / (1024.0 * 1024.0)) << " MiB of arenas\n";
}

void ChunkMeshManager::Update(const std::vector<glm::vec2>& activeChunks) {
	// Prune first so slots freed this frame are immediately available to the
	// chunks we are about to generate.
	PruneChunks(activeChunks);
	GenerateChunks(activeChunks);
	// Finish any block readbacks whose GPU copy completed; never blocks.
	PollReadbacks();
}

void ChunkMeshManager::PruneChunks(const std::vector<glm::vec2>& activeChunks) {
	// Drop every resident chunk that has left the active window and hand its
	// pool slot back, so a newly-visible chunk can reuse it. Without this the
	// map (and pool usage) would grow without bound as the player explores;
	// with it, usage stays bounded to roughly one view-distance disc.
	std::unordered_set<ChunkCoord> stillActive(activeChunks.begin(), activeChunks.end());
	for (auto it = _chunkMap.begin(); it != _chunkMap.end(); ) {
		if (stillActive.find(it->first) == stillActive.end()) {
			const ChunkResident& res = it->second;
			if (res.poolId >= 0) _pools[res.poolId].Release(res.slot); // recycle the slot
			it = _chunkMap.erase(it);
		} else {
			++it;
		}
	}
}

// Copy one freshly-generated chunk's geometry out of its private GPU buffers and
// into a borrowed pool slot. The returned record is all we keep; the caller then
// lets the source VoxelCubeMesh die, which frees its per-chunk buffers.
ChunkResident ChunkMeshManager::MigrateToPool(Core::VoxelCubeMesh& mesh) {
	ChunkResident resident;
	resident.offset = mesh.offset;
	resident.quadCount = mesh.maxQuards; // maxQuards holds this chunk's exact quad count
	const int quadCount = resident.quadCount;
	if (quadCount <= 0) return resident; // empty chunk: nothing to store, poolId stays -1

	// Borrow a slot from the smallest bucket that fits. If that tier happens to
	// be full, spill into the next-larger tier (wastes a little memory, but a
	// rendered chunk beats a hole). Holes only return if every fitting tier is
	// simultaneously full, or the chunk is bigger than the largest bucket.
	int poolId = -1;
	int slot = -1;
	for (int i = 0; i < (int)_pools.size(); ++i) {
		if (quadCount > _pools[i].BucketQuads()) continue; // too big for this tier
		const int s = _pools[i].Claim();
		if (s >= 0) { poolId = i; slot = s; break; }
		// tier full -> fall through and try the next larger tier
	}
	if (poolId < 0) {
		if (!_warnedNoSlot) {
			std::cerr << "[ChunkPool] no free slot for a " << quadCount
			          << "-quad chunk (all fitting tiers full, or chunk exceeds the largest bucket); not rendered\n";
			_warnedNoSlot = true;
		}
		return resident;
	}

	ChunkPool& pool = _pools[poolId];

	// The geometry compute shader wrote these buffers incoherently; make those
	// writes visible to the upcoming GPU->GPU copies (the shader's own barrier
	// did not include the buffer-update bit that glCopyBufferSubData needs).
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

	// Packed vertex data -> the slot's byte range in the packed arena.
	glBindBuffer(GL_COPY_READ_BUFFER, mesh.packedData_SSBO);
	glBindBuffer(GL_COPY_WRITE_BUFFER, pool.PackedArena());
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0,
		pool.PackedByteOffset(slot), (GLsizeiptr)quadCount * kPackedBytesPerQuad);

	// Index data (chunk-local 0-based) -> the slot's byte range in the index arena.
	glBindBuffer(GL_COPY_READ_BUFFER, mesh.ibo);
	glBindBuffer(GL_COPY_WRITE_BUFFER, pool.IboArena());
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0,
		pool.IboByteOffset(slot), (GLsizeiptr)quadCount * kIndexBytesPerQuad);

	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

	// Record this chunk's world offset in the pool's per-slot offset table. The
	// vertex shader reads it by slot: each draw command sets baseInstance = slot,
	// and the shader indexes chunkOffsets[gl_BaseInstanceARB]. Writing it once
	// here - rather than re-uploading every offset every frame - is what lets the
	// per-frame draw path touch only the command buffer.
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pool.OffsetSSBO());
	glBufferSubData(GL_SHADER_STORAGE_BUFFER,
		static_cast<GLintptr>(slot) * 2 * static_cast<GLintptr>(sizeof(float)),
		2 * static_cast<GLsizeiptr>(sizeof(float)), &resident.offset[0]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	resident.poolId = poolId;
	resident.slot = slot;
	return resident;
}

void ChunkMeshManager::GenerateChunks(const std::vector<glm::vec2>& activeChunks) {
	int numOfGen = 0;
	for (const auto& coord : activeChunks) {
		// Generate if not yet stored
		if (_chunkMap.find(coord) == _chunkMap.end()) {
			glm::vec2 offset = coord * glm::vec2(_width, _depth);

			std::unique_ptr<Core::VoxelCubeMesh> voxelData = Core::CreateVoxelCubes3DMesh(_width, _height, _depth, offset, false, _amplitude, _frequency, _persistance, _lacunarity, _octave, true);
			// Copy the geometry into a pool slot, then let voxelData die at the
			// end of this iteration so its per-chunk GPU buffers are reclaimed.
			_chunkMap[coord] = MigrateToPool(*voxelData);

			// Start reading this chunk's block IDs back to the CPU for physics.
			// Done while voxelData (and its blockID_SSBO) is still alive, and for
			// every chunk - a chunk with no visible faces still has solid blocks.
			EnqueueBlockReadback(coord, *voxelData);

			numOfGen++;
			if (numOfGen >= 15) break; // Limit generation per frame to maintain FPS
		}
	}
}

void ChunkMeshManager::DeleteChunk(glm::vec2 coord) {
	auto it = _chunkMap.find(coord);
	if (it == _chunkMap.end()) return;
	const ChunkResident& res = it->second;
	if (res.poolId >= 0) _pools[res.poolId].Release(res.slot); // hand the slot back
	_chunkMap.erase(it);
}

void ChunkMeshManager::EnqueueBlockReadback(glm::vec2 coord, Core::VoxelCubeMesh& mesh) {
	if (!_blockSink) return;             // nobody consuming -> skip the work
	if (mesh.blockID_SSBO == 0) return;  // no block buffer (should not happen)

	// The block IDs were written by compute shaders earlier in generation. This
	// barrier makes those writes visible to the glGetBufferSubData in PollReadbacks().
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

	// Take ownership of the block buffer instead of copying it. Nulling the mesh's
	// handle stops its destructor (which runs at the end of this generation iteration)
	// from deleting the buffer, so it stays alive until PollReadbacks() reads and frees
	// it. This drops the GPU->GPU copy entirely - which on this driver was delivering
	// all-air data, most likely because the source SSBO was freed before the deferred
	// copy resolved.
	GLuint blockSSBO = mesh.blockID_SSBO;
	mesh.blockID_SSBO = 0;

	// Fence marks "GPU has finished everything up to here". PollReadbacks() checks
	// it with a zero timeout, so the render thread never waits on the GPU.
	GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	_pendingReadbacks.push_back({ coord, blockSSBO, fence });
}

void ChunkMeshManager::PollReadbacks() {
	if (_pendingReadbacks.empty()) return;

	const int paddedW = _width + 2;
	const int paddedH = _height + 2;
	const size_t interiorCount = (size_t)_width * _height * _depth;

	for (auto it = _pendingReadbacks.begin(); it != _pendingReadbacks.end(); ) {
		// FLUSH_COMMANDS_BIT makes this first poll flush the generation work + fence to
		// the GPU (like glFlush). Without it a fence can sit unsignaled forever if its
		// commands never got flushed, so the readback would never complete and the
		// world thread would never receive block data. Timeout 0 still never blocks.
		const GLenum status = glClientWaitSync(it->fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
		if (status != GL_ALREADY_SIGNALED && status != GL_CONDITION_SATISFIED) {
			++it; // GPU not done generating yet - leave it and check again next frame
			continue;
		}

		// Read the whole padded buffer straight into CPU memory. glGetBufferSubData
		// avoids glMapBuffer (which returned null on this driver); the fence above
		// guarantees the GPU finished writing the block IDs, so this read does not stall.
		const int paddedD = _depth + 2;
		std::vector<uint16_t> padded((size_t)paddedW * paddedH * paddedD);
		glBindBuffer(GL_COPY_READ_BUFFER, it->blockSSBO);
		glGetBufferSubData(GL_COPY_READ_BUFFER, 0,
			(GLsizeiptr)padded.size() * sizeof(uint16_t), padded.data());
		glBindBuffer(GL_COPY_READ_BUFFER, 0);

		// Deinterleave: drop the 1-voxel padding skirt so physics indexes clean
		// 0-based interior coordinates (x + y*width + z*width*height). Each
		// interior x-run is contiguous in both buffers, so copy it as one row.
		std::vector<uint16_t> interior(interiorCount);
		for (int z = 0; z < _depth; ++z) {
			for (int y = 0; y < _height; ++y) {
				const size_t dst = (size_t)y * _width + (size_t)z * _width * _height;
				const size_t srcRow = (size_t)(y + 1) * paddedW
					+ (size_t)(z + 1) * paddedW * paddedH + 1; // +1 skips the x-skirt
				std::memcpy(&interior[dst], &padded[srcRow], (size_t)_width * sizeof(uint16_t));
			}
		}
		_blockSink->Push(it->coord, std::move(interior));

		glDeleteSync(it->fence);
		glDeleteBuffers(1, &it->blockSSBO); // done with the block buffer; free it now
		it = _pendingReadbacks.erase(it);
	}
}
