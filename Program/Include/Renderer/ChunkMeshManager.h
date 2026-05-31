#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <glm.hpp>

#include "Core.h"

using ChunkCoord = glm::vec2;

#include "Helpers/HashHelpers.h"
#include "Helpers/BlockDataQueue.h"
#include "Renderer/ChunkPool.h"

// Once a chunk's geometry has been copied into a pool slot, this lightweight
// record is all we keep per chunk on the CPU side (it owns no GPU handles).
struct ChunkResident {
	int poolId = -1;                     // which pool, or -1 if empty chunk / no slot available
	int slot = -1;                       // slot index inside that pool
	int quadCount = 0;                   // quads stored in the slot
	glm::vec2 offset = glm::vec2(0.0f);  // world offset, fed to the "offset" draw uniform
};

class ChunkMeshManager {
public:

	ChunkMeshManager();

	ChunkMeshManager(const ChunkMeshManager&) = delete;
	ChunkMeshManager& operator=(const ChunkMeshManager&) = delete;

	ChunkMeshManager(ChunkMeshManager&&) = default;
	ChunkMeshManager& operator=(ChunkMeshManager&&) = default;

	void Update(const std::vector<glm::vec2>& activeChunks);

	void GenerateChunks(const std::vector<glm::vec2>& activeChunks);
	void PruneChunks(const std::vector<glm::vec2>& activeChunks);

	// Where finished GPU->CPU block readbacks are pushed (owned by App, drained by
	// the world thread). Null disables readback entirely - we skip the GPU copy.
	void SetBlockDataSink(BlockDataQueue* sink) { _blockSink = sink; }

	// Advance any in-flight block readbacks: poll their fences without blocking and
	// hand off the ones whose GPU copy has finished. Render thread only.
	void PollReadbacks();

	void DestroyChunks() {
		_chunkMap.clear();
		_pools.clear(); // free pool arenas/VAOs while the GL context is still current

		// Free the owned block buffers and outstanding fences while the context is current.
		for (PendingReadback& pr : _pendingReadbacks) {
			if (pr.fence) glDeleteSync(pr.fence);
			if (pr.blockSSBO) glDeleteBuffers(1, &pr.blockSSBO);
		}
		_pendingReadbacks.clear();
	}

	glm::vec2 GetChunkCoordFromPosition(const glm::vec3& position) const {
		return glm::vec2(
			std::floor(position.x / _width),
			std::floor(position.z / _depth)
		);
	}

	void UpdateSettings(float scale, float amplitude, float frequency, int octaves, float lacunarity, float persistance, int width, int height,int depth, int viewDistance) {
		_scale = scale;
		_amplitude = amplitude;
		_frequency = frequency;
		_octave = octaves;
		_lacunarity = lacunarity;
		_persistance = persistance;
		_width = width;
		_height = height;
		_depth = depth;
		_viewDistance = viewDistance;

	}

	std::unordered_map<ChunkCoord, ChunkResident>& GetChunkMap() {
		return _chunkMap;
	}

	ChunkPool& GetPool(int poolId) {
		return _pools[poolId];
	}

	int PoolCount() const { return (int)_pools.size(); }

private:
	// A chunk's block buffer awaiting CPU readback. Rather than copy blockID_SSBO
	// into a scratch buffer, we take ownership of the buffer itself and keep it alive
	// here. We poll `fence` each frame and only read `blockSSBO` once it signals, so
	// the render thread never stalls waiting on the GPU.
	struct PendingReadback {
		glm::vec2 coord;
		GLuint    blockSSBO = 0;
		GLsync    fence     = nullptr;
	};

	std::unordered_map<ChunkCoord, ChunkResident> _chunkMap;
	std::vector<ChunkPool> _pools;   // segregated fixed-bucket geometry pools
	bool _warnedNoSlot = false;      // throttles the "no slot available" log to once

	BlockDataQueue* _blockSink = nullptr;        // where finished readbacks go (App owns it)
	std::vector<PendingReadback> _pendingReadbacks; // block buffers in flight, awaiting their fence

	float _scale = 0.1f;
	float _amplitude = 1.0f;
	float _frequency = 0.1f;
	int _octave = 5;
	float _lacunarity = 2.0f;
	float _persistance = 0.5f;
	int _width = 16;
	int _height = 256;
	int _depth = 16;
	int _viewDistance = 24;

	void DeleteChunk(glm::vec2 coord);
	ChunkResident MigrateToPool(Core::VoxelCubeMesh& mesh);

	// Queue a non-stalling GPU->CPU readback of this chunk's block IDs: take ownership
	// of blockID_SSBO (so the mesh's destructor won't free it) and drop a fence.
	// PollReadbacks() reads the buffer and frees it on a later frame, once the fence
	// signals. Done for every generated chunk (even empty geometry), since physics
	// still needs the block data.
	void EnqueueBlockReadback(glm::vec2 coord, Core::VoxelCubeMesh& mesh);

};
