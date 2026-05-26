#pragma once

#include <unordered_set>
#include <vector>
#include <mutex>
#include <glm.hpp>
#include "Helpers/HashHelpers.h"

class ChunkBlockManager {
public:
    ChunkBlockManager() = default;

    void UpdateActiveWindow(const glm::vec3& playerPosition, int viewDistance);

    std::vector<glm::vec2> GetActiveChunksSnapshot();

    void SetChunkDimensions(int width, int depth) {
        _chunkWidth = width;
        _chunkDepth = depth;
    }

private:
    std::unordered_set<glm::vec2> _activeChunks;
    std::vector<glm::vec2> _activeChunksOrdered;
    std::mutex _chunksMutex;

    int _chunkWidth = 16;
    int _chunkDepth = 16;

    glm::vec2 GetChunkCoordFromPosition(const glm::vec3& position) const {
        return glm::vec2(
            std::floor(position.x / _chunkWidth),
            std::floor(position.z / _chunkDepth)
        );
    }
};
