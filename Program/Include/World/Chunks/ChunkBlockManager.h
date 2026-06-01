#pragma once

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <cstdint>
#include <glm.hpp>
#include "Helpers/HashHelpers.h"
#include "Helpers/Config.h"

class ChunkBlockManager {
public:
    ChunkBlockManager() {
        _chunkWidth = Config::Get().chunkWidth;
        _chunkHeight = Config::Get().chunkHeight;
        _chunkDepth = Config::Get().chunkDepth;
    }

    void UpdateActiveWindow(const glm::vec3& playerPosition, int viewDistance);

    std::vector<glm::vec2> GetActiveChunksSnapshot();

    // Store a chunk's interior block IDs, read back from the GPU on the render
    // thread and handed over via the BlockDataQueue. World thread only.
    void IngestBlockData(glm::vec2 coord, std::vector<uint16_t>&& blockIDs);

    // Look up a single block. Returns 0 (air) for out-of-range coordinates or a
    // chunk whose data has not arrived yet. Lock-free: only the world thread ever
    // touches the block-data map (ingest here, physics reads, eviction).
    uint16_t GetBlock(glm::vec2 coord, int x, int y, int z) const;

    bool HasBlockData(glm::vec2 coord) const {
        return _blockData.find(coord) != _blockData.end();
    }

    // Collision query: true if any solid voxel overlaps the world-space AABB
    // [boxMin, boxMax]. Uses the rendered geometry's world<->voxel mapping
    // (interior voxel (lx,ly,lz) of chunk (cx,cz) spans world X [cx*16+lx+1,+1],
    // Z [cz*16+lz+1,+1], Y [ly-1, ly]). World thread only.
    bool IsBoxColliding(const glm::vec3& boxMin, const glm::vec3& boxMax) const;

    // True if the chunk whose interior contains worldPos has streamed in. Used to
    // freeze gravity over not-yet-loaded terrain so the player doesn't fall through.
    bool IsColumnLoadedAtWorld(const glm::vec3& worldPos) const;

    void SetChunkDimensions(int width, int height, int depth) {
        _chunkWidth = width;
        _chunkHeight = height;
        _chunkDepth = depth;
    }

private:
    std::unordered_set<glm::vec2> _activeChunks;
    std::vector<glm::vec2> _activeChunksOrdered;
    std::mutex _chunksMutex;

    // coord -> interior block IDs (size _chunkWidth*_chunkHeight*_chunkDepth).
    // Touched only by the world thread, so it needs no mutex.
    std::unordered_map<glm::vec2, std::vector<uint16_t>> _blockData;

    int _chunkWidth = 16;
    int _chunkHeight = 256;
    int _chunkDepth = 16;

    glm::vec2 GetChunkCoordFromPosition(const glm::vec3& position) const {
        return glm::vec2(
            std::floor(position.x / _chunkWidth),
            std::floor(position.z / _chunkDepth)
        );
    }
};
