#include "World/Chunks/ChunkBlockManager.h"
#include <cmath>

void ChunkBlockManager::UpdateActiveWindow(const glm::vec3& playerPosition, int viewDistance) {
    glm::vec2 playerChunk = GetChunkCoordFromPosition(playerPosition);
    
    std::unordered_set<glm::vec2> newActiveChunksSet;
    std::vector<glm::vec2> newActiveChunksOrdered;

    // Spiral algorithm: starts at (0,0) and expands outwards in layers
    // This ensures closer chunks are added to the list first.
    newActiveChunksSet.insert(playerChunk);
    newActiveChunksOrdered.push_back(playerChunk);

    for (int d = 1; d <= viewDistance; ++d) {
        // Top edge
        for (int x = -d; x <= d; ++x) {
            glm::vec2 coord = playerChunk + glm::vec2(x, d);
            if (x * x + d * d <= viewDistance * viewDistance) {
                newActiveChunksSet.insert(coord);
                newActiveChunksOrdered.push_back(coord);
            }
        }
        // Right edge
        for (int z = d - 1; z >= -d; --z) {
            glm::vec2 coord = playerChunk + glm::vec2(d, z);
            if (d * d + z * z <= viewDistance * viewDistance) {
                newActiveChunksSet.insert(coord);
                newActiveChunksOrdered.push_back(coord);
            }
        }
        // Bottom edge
        for (int x = d - 1; x >= -d; --x) {
            glm::vec2 coord = playerChunk + glm::vec2(x, -d);
            if (x * x + d * d <= viewDistance * viewDistance) {
                newActiveChunksSet.insert(coord);
                newActiveChunksOrdered.push_back(coord);
            }
        }
        // Left edge
        for (int z = -d + 1; z < d; ++z) {
            glm::vec2 coord = playerChunk + glm::vec2(-d, z);
            if (d * d + z * z <= viewDistance * viewDistance) {
                newActiveChunksSet.insert(coord);
                newActiveChunksOrdered.push_back(coord);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(_chunksMutex);
        _activeChunks = std::move(newActiveChunksSet);
        _activeChunksOrdered = std::move(newActiveChunksOrdered);
    }
}

std::vector<glm::vec2> ChunkBlockManager::GetActiveChunksSnapshot() {
    std::lock_guard<std::mutex> lock(_chunksMutex);
    return _activeChunksOrdered;
}
