#pragma once

#include <unordered_map>
#include <unordered_set>
#include <glm.hpp>

#include "Core.h"

using ChunkCoord = glm::vec2;

#include "Helpers/HashHelpers.h"

class ChunkMeshManager {
public:

	ChunkMeshManager() {}

	ChunkMeshManager(const ChunkMeshManager&) = delete;
	ChunkMeshManager& operator=(const ChunkMeshManager&) = delete;

	ChunkMeshManager(ChunkMeshManager&&) = default;
	ChunkMeshManager& operator=(ChunkMeshManager&&) = default;

	void Update(const std::vector<glm::vec2>& activeChunks);

	void GenerateChunks(const std::vector<glm::vec2>& activeChunks);
	// void PruneChunks(const glm::vec3& position); // Disabled as per plan

	void DestroyChunks() {
		_chunkMap.clear();
	}
	
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

	std::unordered_map<ChunkCoord, std::unique_ptr<Core::VoxelCubeMesh>>& GetChunkMap() {
		return _chunkMap;
	}

private:
	std::unordered_map<ChunkCoord, std::unique_ptr<Core::VoxelCubeMesh>> _chunkMap;

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

	void DeleteChunk(glm::vec2 coord);

};