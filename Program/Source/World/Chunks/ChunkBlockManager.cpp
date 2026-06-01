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

    // Evict CPU block data for chunks that left the window, so memory stays bounded
    // to roughly one view-distance disc. No lock: _blockData is world-thread-only.
    for (auto it = _blockData.begin(); it != _blockData.end(); ) {
        if (newActiveChunksSet.find(it->first) == newActiveChunksSet.end()) {
            it = _blockData.erase(it);
        } else {
            ++it;
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

void ChunkBlockManager::IngestBlockData(glm::vec2 coord, std::vector<uint16_t>&& blockIDs) {
    _blockData[coord] = std::move(blockIDs);
}

uint16_t ChunkBlockManager::GetBlock(glm::vec2 coord, int x, int y, int z) const {
    if (x < 0 || y < 0 || z < 0 ||
        x >= _chunkWidth || y >= _chunkHeight || z >= _chunkDepth) {
        return 0; // outside the chunk -> air
    }
    auto it = _blockData.find(coord);
    if (it == _blockData.end()) return 0; // not loaded yet -> treat as air
    const size_t index = (size_t)x + (size_t)y * _chunkWidth
        + (size_t)z * _chunkWidth * _chunkHeight;
    return it->second[index];
}

bool ChunkBlockManager::IsBoxColliding(const glm::vec3& boxMin, const glm::vec3& boxMax) const {
    // Global voxel index ranges that could overlap the box. Voxel gx spans world
    // X [gx+1, gx+2), gz spans Z [gz+1, gz+2), ly spans Y [ly-1, ly). The bounds
    // below are deliberately generous; the per-voxel overlap test rejects extras.
    const int gxLo = (int)std::floor(boxMin.x) - 2;
    const int gxHi = (int)std::floor(boxMax.x);
    const int gzLo = (int)std::floor(boxMin.z) - 2;
    const int gzHi = (int)std::floor(boxMax.z);
    const int lyLo = (int)std::floor(boxMin.y) - 2;
    const int lyHi = (int)std::floor(boxMax.y) + 2;

    for (int gx = gxLo; gx <= gxHi; ++gx) {
        const float vMinX = (float)gx + 1.0f;
        const float vMaxX = (float)gx + 2.0f;
        if (boxMax.x <= vMinX || boxMin.x >= vMaxX) continue;

        const int cx = (int)std::floor((double)gx / _chunkWidth);
        const int lx = gx - cx * _chunkWidth;

        for (int gz = gzLo; gz <= gzHi; ++gz) {
            const float vMinZ = (float)gz + 1.0f;
            const float vMaxZ = (float)gz + 2.0f;
            if (boxMax.z <= vMinZ || boxMin.z >= vMaxZ) continue;

            const int cz = (int)std::floor((double)gz / _chunkDepth);
            const int lz = gz - cz * _chunkDepth;

            for (int ly = lyLo; ly <= lyHi; ++ly) {
                const float vMinY = (float)ly - 1.0f;
                const float vMaxY = (float)ly;
                if (boxMax.y <= vMinY || boxMin.y >= vMaxY) continue;

                if (GetBlock(glm::vec2((float)cx, (float)cz), lx, ly, lz) != 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool ChunkBlockManager::IsColumnLoadedAtWorld(const glm::vec3& worldPos) const {
    const int gx = (int)std::floor(worldPos.x) - 1;
    const int gz = (int)std::floor(worldPos.z) - 1;
    const int cx = (int)std::floor((double)gx / _chunkWidth);
    const int cz = (int)std::floor((double)gz / _chunkDepth);
    return HasBlockData(glm::vec2((float)cx, (float)cz));
}
