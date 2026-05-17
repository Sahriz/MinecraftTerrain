#include "Renderer/ChunkRenderer.h"

void ChunkRenderer::UpdateActiveChunk(const glm::vec3& position, ChunkMeshManager& chunkManager) {
	glm::vec2 playerChunk = chunkManager.GetChunkCoordFromPosition(position);	
	_previousFrameActiveChunkSet = _activeChunkSet;
	_activeChunkSet.clear();
	auto& chunkMap = chunkManager.GetChunkMap();
	for (int x = -_viewDistance; x <= _viewDistance; x++) {
			for (int z = -_viewDistance; z <= _viewDistance; z++) {
				if (glm::abs(x * z) > _viewDistance * _viewDistance / 1.5f) continue;
				glm::vec2 coord = playerChunk + glm::vec2(x, z);
				//std::cout << coord.x << " " << coord.y << " " << coord.z << "\n";
				
				if (chunkMap.find(coord) != chunkMap.end()) {
					_activeChunkSet.insert(coord);
					// Generate if not yet stored
					if (!chunkMap[coord].get()->gpuLoaded) {
						//SetupChunkRenderData(*chunkMap[coord]);
					}
				}
			}
		}	
}

void ChunkRenderer::SetupChunkRenderData(Core::VoxelCubeMesh& mesh) {
	
	glBindVertexArray(mesh.vao);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.packedData_SSBO);

	glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(uint32_t), (void*)0);
	glEnableVertexAttribArray(0);

	// IMPORTANT: Bind the index buffer to the VAO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);

	glBindVertexArray(0);
	mesh.gpuLoaded = true;
}

