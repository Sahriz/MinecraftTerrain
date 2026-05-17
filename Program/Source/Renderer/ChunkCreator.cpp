#include "Renderer/ChunkCreator.h"


std::unique_ptr<Core::VoxelCubeMesh> ChunkCreator::GenerateChunk(const glm::vec2& coord) {

	glm::vec2 offset = coord * glm::vec2(_width, _depth);
	//Try generating the mesh with and without GPU to see the difference in speed! The function call is the same but the end of
	//the function call is GPU for the gpu implementtion. Please do keep in mind the noise map is still using compute shaders
	//even on the cpu implementation, so that is technically a speedup that should not be granted as a possitive for the CPU part
	//of this code. 
	std::unique_ptr<Core::VoxelCubeMesh> voxelData = Core::CreateVoxelCubes3DMesh(_width, _height, _depth, offset, false, _amplitude, _frequency, _persistance, _lacunarity, _octave, true);
	return std::move(voxelData);
}
