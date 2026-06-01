#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <cstdint>
#include <glm.hpp>

#include "Core.h"

#include "Helpers/Config.h"

using ChunkCoord = glm::vec2;

#include "Helpers/HashHelpers.h"
#include "Helpers/BlockDataQueue.h"
#include "Renderer/ChunkPool.h"

// All we keep per chunk on the CPU once its geometry is in a pool slot.
// Owns no GPU handles.
struct ChunkResident {
	int poolId = -1;                     // which terrain pool, or -1 if empty chunk / no slot available
	int slot = -1;                       // slot index inside that pool
	int quadCount = 0;                   // terrain quads stored in the slot
	// Parallel water residency. Stays -1 / 0 when the chunk has no water, so dry
	// chunks don't burn a water slot.
	int waterPoolId = -1;                // which water pool, or -1 if no water / no slot
	int waterSlot = -1;                  // slot index inside that water pool
	int waterQuadCount = 0;              // water quads stored in the slot
	glm::vec2 offset = glm::vec2(0.0f);  // world offset, fed to the "offset" draw uniform
};

class ChunkMeshManager {
public:

	ChunkMeshManager();
	~ChunkMeshManager();

	ChunkMeshManager(const ChunkMeshManager&) = delete;
	ChunkMeshManager& operator=(const ChunkMeshManager&) = delete;

	ChunkMeshManager(ChunkMeshManager&&) = default;
	ChunkMeshManager& operator=(ChunkMeshManager&&) = default;

	void Update(const std::vector<glm::vec2>& activeChunks);

	void GenerateChunks(const std::vector<glm::vec2>& activeChunks);
	void PruneChunks(const std::vector<glm::vec2>& activeChunks);

	// Where finished GPU->CPU block readbacks go (App owns it, world thread drains it).
	// Null disables readback entirely.
	void SetBlockDataSink(BlockDataQueue* sink) { _blockSink = sink; }

	// Advance any in-flight block readbacks: poll their fences without blocking and
	// hand off the ones whose GPU copy has finished. Render thread only.
	void PollReadbacks();

	// Finish chunk generations whose fence has signaled: read the actual quad counts
	// (no stall, the fence guarantees the GPU is done) and migrate the geometry into
	// pool slots. Render thread only.
	void PollGenerations();

	// Delete meshes that were held for migration/generation once the GPU is done.
	void PollCleanups();

	void DestroyChunks();

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

	// Water (transparent) pools, drawn in a separate blended pass after the opaque terrain.
	ChunkPool& GetWaterPool(int poolId) {
		return _waterPools[poolId];
	}

	int WaterPoolCount() const { return (int)_waterPools.size(); }

private:
	// A chunk's block buffer awaiting CPU readback. We take ownership of blockID_SSBO
	// (instead of copying it) and poll `fence` each frame, reading `blockSSBO` only
	// once it signals so the render thread never stalls on the GPU.
	struct PendingReadback {
		glm::vec2 coord;
		GLuint    blockSSBO = 0;
		GLsync    fence     = nullptr;
	};

	// A chunk whose GPU geometry has been issued but not yet copied into a pool slot.
	// We hold the whole mesh (its packed/index buffers + vertex counters) until `fence`
	// signals, then read the counts and migrate. Keeps generation off the stall path.
	struct PendingGeneration {
		glm::vec2 coord;
		std::unique_ptr<Core::VoxelCubeMesh> mesh;
		GLsync fence = nullptr;
	};

	// A mesh that has finished its generation or migration but whose GPU source 
	// buffers must remain alive until the GPU has finished reading them.
	struct PendingCleanup {
		std::unique_ptr<Core::VoxelCubeMesh> mesh;
		GLsync fence = nullptr;
	};

	std::unordered_map<ChunkCoord, ChunkResident> _chunkMap;
	std::vector<ChunkPool> _pools;        // segregated fixed-bucket terrain geometry pools
	std::vector<ChunkPool> _waterPools;   // parallel, smaller pools for transparent water geometry
	bool _warnedNoSlot = false;           // throttles the terrain "no slot available" log to once
	bool _warnedNoWaterSlot = false;      // throttles the water "no slot available" log to once

	BlockDataQueue* _blockSink = nullptr;        // where finished readbacks go (App owns it)
	std::vector<PendingReadback> _pendingReadbacks; // block buffers in flight, awaiting their fence

	std::vector<PendingGeneration> _pendingGenerations; // chunks generated, awaiting fence + migration
	std::unordered_set<ChunkCoord> _pendingGenCoords;   // coords mid-generation, so we don't re-kick them
	std::vector<PendingCleanup>    _pendingCleanups;    // meshes whose buffers are in flight (copy/read)

	// Scratch & Block pooling members
	void InitScratchPool(int maxTerrainQuads, int maxWaterQuads);
	void DestroyScratchPool();
	int ClaimScratchSlot();
	void ReturnScratchSlot(int slot);

	GLuint ClaimBlockBuffer();
	void ReturnBlockBuffer(GLuint buffer);

	std::vector<Core::ScratchMesh> _scratchPool;
	std::vector<int>              _freeScratchSlots;
	std::vector<GLuint>           _blockBufferPool;
	std::vector<GLuint>           _freeBlockBuffers;

	float _scale = 0.1f;
	float _amplitude = Config::Get().noiseAmplitude;
	float _frequency = Config::Get().noiseFrequency;
	int _octave = Config::Get().noiseOctaves;
	float _lacunarity = Config::Get().noiseLacunarity;
	float _persistance = Config::Get().noisePersistence;
	int _width = Config::Get().chunkWidth;
	int _height = Config::Get().chunkHeight;
	int _depth = Config::Get().chunkDepth;
	int _viewDistance = Config::Get().viewDistance;

	void DeleteChunk(glm::vec2 coord);
	ChunkResident MigrateToPool(Core::VoxelCubeMesh& mesh);

	// Queue a non-stalling GPU->CPU readback of this chunk's block IDs: take ownership
	// of blockID_SSBO (so the mesh destructor won't free it) and drop a fence.
	// PollReadbacks() reads and frees it on a later frame once the fence signals.
	// Done for every chunk, even empty ones, since physics needs the block data.
	void EnqueueBlockReadback(glm::vec2 coord, Core::VoxelCubeMesh& mesh);

};
