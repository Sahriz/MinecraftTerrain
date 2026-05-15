#include "Chunks/ChunkManager.h"

void ChunkManager::Update(const glm::vec3& position) {
	GenerateChunk(position);
	//PruneChunks(position);
}

void ChunkManager::GenerateChunk(const glm::vec3& position) {
	
	glm::vec2 playerChunk = GetChunkCoordFromPosition(position);
	bool generated = false;
	int numOfGen = 0;
	for (int x = -_viewDistance; x <= _viewDistance; x++) {
			for (int z = -_viewDistance; z <= _viewDistance; z++) {
				if (glm::abs(x * z) > _viewDistance * _viewDistance / 1.5f) continue;
				glm::vec2 coord = playerChunk + glm::vec2(x, z);
				//std::cout << coord.x << " " << coord.y << " " << coord.z << "\n";
				// Generate if not yet stored
				if (_chunkMap.find(coord) == _chunkMap.end() ) {
					glm::vec2 offset = coord * glm::vec2(_width, _depth);
					//Try generating the mesh with and without GPU to see the difference in speed! The function call is the same but the end of
					//the function call is GPU for the gpu implementtion. Please do keep in mind the noise map is still using compute shaders
					//even on the cpu implementation, so that is technically a speedup that should not be granted as a possitive for the CPU part
					//of this code. 
					std::unique_ptr<Core::VoxelCubeMesh> voxelData = Core::CreateVoxelCubes3DMesh(_width, _height, _depth, offset, false, _amplitude, _frequency, _persistance, _lacunarity, _octave, true);
					_chunkMap[coord] = std::move(voxelData);
					generated = true;
					numOfGen++;
					if (numOfGen > 0) break;
				}
			}
	}
	
}

void ChunkManager::PruneChunks(const glm::vec3& position) {
	glm::vec2 playerChunk = GetChunkCoordFromPosition(position);
	float maxDist = _viewDistance + 2.0f; // Buffer zone

	for (auto it = _chunkMap.begin(); it != _chunkMap.end(); ) {
		float dist = glm::distance(playerChunk, it->first);
		if (dist > maxDist) {
			DeleteChunk(it->first); // This calls glDeleteBuffers
		}
		else {
			++it;
		}
	}
}

void ChunkManager::DestroyChunks() {
	_chunkMap.clear();
}

void ChunkManager::DeleteChunk(glm::vec2 coord) {
	_chunkMap.erase(coord);
}