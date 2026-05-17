#pragma once

#include "glm.hpp"
#include "Core.h"

class ChunkCreator {

public:


	std::unique_ptr<Core::VoxelCubeMesh> GenerateChunk(const glm::vec2& position);


private:
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
};