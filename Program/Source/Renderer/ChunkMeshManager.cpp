#include "Renderer/ChunkMeshManager.h"

#include <iostream>
#include <cstring>

namespace {
	// Size-class tiers, smallest bucket first (MigrateToPool relies on that order).
	// A chunk borrows a slot from the smallest bucket that fits its quad count, so a
	// flat 300-quad chunk doesn't reserve a 10k-quad slot. Finer tiers waste fewer
	// bytes per slot, so the same memory buys more slots and fewer on-screen holes.
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

	// Parallel water tiers (~63 MiB). Water meshes are usually just the flat top
	// surface of a sea/lake plus shore edges, so buckets are smaller than terrain's
	// but plentiful. Smallest bucket first (MigrateToPool relies on that order).
	constexpr PoolTier kWaterTiers[] = {
		{  256, 1024 },  //   7.00 MiB - typical sea/lake top surface
		{ 1024,  768 },  //  21.00 MiB
		{ 4096,  256 },  //  28.00 MiB - big shorelines / complex coasts
		{ 8192,   32 },  //   7.00 MiB - rare, very broken water chunks
	};
	constexpr int kWaterTierCount = (int)(sizeof(kWaterTiers) / sizeof(kWaterTiers[0]));

	// Mirrors ChunkPool's internal layout: 16 bytes packed + 12 bytes index per quad.
	constexpr int kPackedBytesPerQuad = 16;
	constexpr int kIndexBytesPerQuad  = 12;
}

ChunkMeshManager::ChunkMeshManager() {
	// Needs a live GL context: App builds the Renderer (which creates it) before this
	// manager, so the arenas can be allocated here.
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

	// Parallel water pools (transparent geometry).
	_waterPools.reserve(kWaterTierCount);
	long long waterBytes = 0;
	int waterSlots = 0;
	for (const PoolTier& t : kWaterTiers) {
		_waterPools.emplace_back(t.bucketQuads, t.slots);
		waterBytes += (long long)t.slots * t.bucketQuads * (kPackedBytesPerQuad + kIndexBytesPerQuad);
		waterSlots += t.slots;
	}
	std::cout << "[ChunkPool] allocated " << _waterPools.size() << " WATER tiers, "
	          << waterSlots << " slots total, ~"
	          << (waterBytes / (1024.0 * 1024.0)) << " MiB of arenas\n";
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
	// Drop residents that left the active window and hand their pool slots back for
	// reuse. Without this the map and pool usage grow without bound as the player
	// explores; with it, usage stays bounded to ~one view-distance disc.
	std::unordered_set<ChunkCoord> stillActive(activeChunks.begin(), activeChunks.end());
	for (auto it = _chunkMap.begin(); it != _chunkMap.end(); ) {
		if (stillActive.find(it->first) == stillActive.end()) {
			const ChunkResident& res = it->second;
			if (res.poolId >= 0) _pools[res.poolId].Release(res.slot);                  // recycle terrain slot
			if (res.waterPoolId >= 0) _waterPools[res.waterPoolId].Release(res.waterSlot); // recycle water slot
			it = _chunkMap.erase(it);
		} else {
			++it;
		}
	}
}

// Copy a freshly-generated chunk's geometry from its private GPU buffers into a
// borrowed pool slot. The returned record is all we keep; the caller then lets the
// source VoxelCubeMesh die, freeing its per-chunk buffers.
ChunkResident ChunkMeshManager::MigrateToPool(Core::VoxelCubeMesh& mesh) {
	ChunkResident resident;
	resident.offset = mesh.offset;
	resident.quadCount = mesh.maxQuards;          // exact terrain quad count
	resident.waterQuadCount = mesh.maxWaterQuads; // exact water quad count (0 for dry chunks)

	// The geometry shaders wrote these buffers incoherently; this barrier makes those
	// writes visible to the GPU->GPU copies below (their own barriers omitted the
	// buffer-update bit that glCopyBufferSubData needs).
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

	// Copy a mesh's packed+index geometry into the smallest fitting slot of the given
	// pool set and record where it landed. Terrain and water share this logic (only
	// the source buffers, pool set, and output fields differ), so one helper keeps
	// the two paths in sync.
	auto migrate = [&](std::vector<ChunkPool>& pools, GLuint packedSrc, GLuint iboSrc,
	                   int quads, bool& warned, const char* label,
	                   int& outPoolId, int& outSlot)
	{
		if (quads <= 0) return; // nothing to store; out fields stay -1

		// Borrow a slot from the smallest bucket that fits; if that tier is full, spill
		// into the next larger one (wastes a little memory, but a drawn chunk beats a
		// hole). A hole appears only if every fitting tier is full, or the chunk is
		// bigger than the largest bucket.
		int poolId = -1;
		int slot = -1;
		for (int i = 0; i < (int)pools.size(); ++i) {
			if (quads > pools[i].BucketQuads()) continue; // too big for this tier
			const int s = pools[i].Claim();
			if (s >= 0) { poolId = i; slot = s; break; }
			// tier full -> fall through and try the next larger tier
		}
		if (poolId < 0) {
			if (!warned) {
				std::cerr << "[ChunkPool] no free " << label << " slot for a " << quads
				          << "-quad chunk (all fitting tiers full, or chunk exceeds the largest bucket); not rendered\n";
				warned = true;
			}
			return;
		}

		ChunkPool& pool = pools[poolId];

		// Packed vertex data -> the slot's byte range in the packed arena.
		glBindBuffer(GL_COPY_READ_BUFFER, packedSrc);
		glBindBuffer(GL_COPY_WRITE_BUFFER, pool.PackedArena());
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0,
			pool.PackedByteOffset(slot), (GLsizeiptr)quads * kPackedBytesPerQuad);

		// Index data (chunk-local 0-based) -> the slot's byte range in the index arena.
		glBindBuffer(GL_COPY_READ_BUFFER, iboSrc);
		glBindBuffer(GL_COPY_WRITE_BUFFER, pool.IboArena());
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0,
			pool.IboByteOffset(slot), (GLsizeiptr)quads * kIndexBytesPerQuad);

		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

		// Record this chunk's world offset in the pool's per-slot table. The shader
		// reads it by slot (baseInstance = slot -> chunkOffsets[gl_BaseInstanceARB]).
		// Writing it once here, not every frame, lets the draw path touch only the
		// command buffer.
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, pool.OffsetSSBO());
		glBufferSubData(GL_SHADER_STORAGE_BUFFER,
			static_cast<GLintptr>(slot) * 2 * static_cast<GLintptr>(sizeof(float)),
			2 * static_cast<GLsizeiptr>(sizeof(float)), &resident.offset[0]);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		outPoolId = poolId;
		outSlot = slot;
	};

	// Opaque terrain goes to the terrain pools, water to the water pools. They're
	// independent: a chunk can have terrain but no water (dry), or water but no
	// visible terrain, and each is stored on its own.
	migrate(_pools, mesh.packedData_SSBO, mesh.ibo,
	        resident.quadCount, _warnedNoSlot, "terrain",
	        resident.poolId, resident.slot);
	migrate(_waterPools, mesh.waterPackedData_SSBO, mesh.waterIbo,
	        resident.waterQuadCount, _warnedNoWaterSlot, "water",
	        resident.waterPoolId, resident.waterSlot);

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

			// Start reading this chunk's block IDs back to the CPU for physics, while
			// voxelData (and its blockID_SSBO) is still alive. Done for every chunk -
			// one with no visible faces still has solid blocks.
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
	if (res.poolId >= 0) _pools[res.poolId].Release(res.slot);                  // hand the terrain slot back
	if (res.waterPoolId >= 0) _waterPools[res.waterPoolId].Release(res.waterSlot); // hand the water slot back
	_chunkMap.erase(it);
}

void ChunkMeshManager::EnqueueBlockReadback(glm::vec2 coord, Core::VoxelCubeMesh& mesh) {
	if (!_blockSink) return;             // nobody consuming -> skip the work
	if (mesh.blockID_SSBO == 0) return;  // no block buffer (should not happen)

	// The block IDs were written by compute shaders earlier in generation. This
	// barrier makes those writes visible to the glGetBufferSubData in PollReadbacks().
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

	// Take ownership of the block buffer instead of copying it. Nulling the mesh's
	// handle stops its destructor from deleting the buffer, so it lives until
	// PollReadbacks() reads and frees it. Dropping the GPU->GPU copy also fixed an
	// all-air readback on this driver, where the source SSBO was freed before the
	// deferred copy resolved.
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
		// FLUSH_COMMANDS_BIT flushes the generation work + fence to the GPU on this
		// first poll (like glFlush). Without it an unflushed fence can sit unsignaled
		// forever and the readback never completes. Timeout 0 still never blocks.
		const GLenum status = glClientWaitSync(it->fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
		if (status != GL_ALREADY_SIGNALED && status != GL_CONDITION_SATISFIED) {
			++it; // GPU not done generating yet - leave it and check again next frame
			continue;
		}

		// Read the whole padded buffer into CPU memory. glGetBufferSubData avoids
		// glMapBuffer (which returned null on this driver); the fence above means the
		// GPU is done writing, so this doesn't stall.
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
