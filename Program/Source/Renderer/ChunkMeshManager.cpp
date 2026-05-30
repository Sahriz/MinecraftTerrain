#include "Renderer/ChunkMeshManager.h"

void ChunkMeshManager::Update(const std::vector<glm::vec2>& activeChunks) {
	GenerateChunks(activeChunks);
}

void ChunkMeshManager::GenerateChunks(const std::vector<glm::vec2>& activeChunks) {
	int numOfGen = 0;
	for (const auto& coord : activeChunks) {
		// Generate if not yet stored
		if (_chunkMap.find(coord) == _chunkMap.end()) {
			glm::vec2 offset = coord * glm::vec2(_width, _depth);
			
			std::unique_ptr<Core::VoxelCubeMesh> voxelData = Core::CreateVoxelCubes3DMesh(_width, _height, _depth, offset, false, _amplitude, _frequency, _persistance, _lacunarity, _octave, true);
			_chunkMap[coord] = std::move(voxelData);
			
			numOfGen++;
			if (numOfGen >= 15) break; // Limit generation per frame to maintain FPS
		}
	}
}

void ChunkMeshManager::DeleteChunk(glm::vec2 coord) {
	_chunkMap.erase(coord);
}