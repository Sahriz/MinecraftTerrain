#pragma once
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "vector"
#include "glm.hpp"


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

namespace Core {

	class AppendBuffer {
	public:
		AppendBuffer(int width, int height, int depth) {
			maxCapacity = width * height * depth;

			// 1. Setup Counter (just 4 bytes)
			glGenBuffers(1, &counterSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, counterSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

			// 2. Setup Data List
			glGenBuffers(1, &dataSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, maxCapacity * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);
		}

		~AppendBuffer() {
			if (counterSSBO) glDeleteBuffers(1, &counterSSBO);
			if (dataSSBO) glDeleteBuffers(1, &dataSSBO);
		}

		AppendBuffer(const AppendBuffer&) = delete; // No copying
		AppendBuffer& operator=(const AppendBuffer&) = delete; // No assignment

		AppendBuffer(AppendBuffer&& other) noexcept : counterSSBO(other.counterSSBO), dataSSBO(other.dataSSBO), maxCapacity(other.maxCapacity) {
			other.counterSSBO = 0;
			other.dataSSBO = 0;
			other.maxCapacity = 0;
		}

		GLuint getCounterSSBO() const { return counterSSBO; }
		GLuint getDataSSBO() const { return dataSSBO; }

	private:
		GLuint counterSSBO;
		GLuint dataSSBO;
		int maxCapacity;
	};

	
	struct VoxelCubeCombinedVertex {
		glm::vec4 position; // 16 bytes
		glm::vec4 normal;   // 16 bytes
		glm::vec2 uv;       // 8 bytes
		float padding[2];   // 8 bytes (Essential to bring the total to 48)
	};
	class VoxelCubeMesh {
	public:
		VoxelCubeMesh() = default;

		// GPU IDs
		GLuint vao = 0, vbo = 0, ibo = 0;
		GLuint blockID_SSBO = 0, distanceToAirSSBO = 0, indirectBuffer = 0, densitySSBO = 0;
		GLuint ssboVertexCounter = 0;
		GLuint stagingVBO = 0, stagingIBO = 0, stagingIndirect = 0;
		GLsync syncObj = nullptr;

		// Logic data
		int indexCount = 0;
		int maxQuards = 0;
		bool gpuLoaded = false;

		VoxelCubeMesh(const VoxelCubeMesh&) = delete;
		VoxelCubeMesh& operator=(const VoxelCubeMesh&) = delete;

		VoxelCubeMesh(VoxelCubeMesh&& other) noexcept {
			*this = std::move(other);
		}

		VoxelCubeMesh& operator=(VoxelCubeMesh&& other) noexcept {
			if (this != &other) {
				// Important: Clean up OUR existing resources before taking new ones
				Release();

				// Steal the values
				vao = other.vao;
				vbo = other.vbo;
				ibo = other.ibo;
				blockID_SSBO = other.blockID_SSBO;
				indirectBuffer = other.indirectBuffer;
				distanceToAirSSBO = other.distanceToAirSSBO;
				densitySSBO = other.densitySSBO;
				ssboVertexCounter = other.ssboVertexCounter;
				stagingVBO = other.stagingVBO;
				stagingIBO = other.stagingIBO;
				stagingIndirect = other.stagingIndirect;
				syncObj = other.syncObj;
				indexCount = other.indexCount;
				maxQuards = other.maxQuards;
				gpuLoaded = other.gpuLoaded;

				// CRITICAL: Set 'other' to 0 so its destructor doesn't delete the buffers we just stole
				other.vao = 0;
				other.vbo = 0;
				other.ibo = 0;
				other.blockID_SSBO = 0;
				other.indirectBuffer = 0;
				other.indirectBuffer = 0;
				other.densitySSBO = 0;
				other.ssboVertexCounter = 0;
				other.stagingVBO = 0;
				other.stagingIBO = 0;
				other.stagingIndirect = 0;
				other.syncObj = nullptr;
				other.gpuLoaded = false;
			}
			return *this;
		}

		// 4. DESTRUCTOR
		~VoxelCubeMesh() {
			Release();
		}
	private:
		void Release() {
			if (vao) glDeleteVertexArrays(1, &vao);
			if (vbo) glDeleteBuffers(1, &vbo);
			if (ibo) glDeleteBuffers(1, &ibo);
			if (blockID_SSBO) glDeleteBuffers(1, &blockID_SSBO);
			if (indirectBuffer) glDeleteBuffers(1, &indirectBuffer);
			if (distanceToAirSSBO) glDeleteBuffers(1, &distanceToAirSSBO);
			if (densitySSBO) glDeleteBuffers(1, &densitySSBO);
			if (stagingVBO) glDeleteBuffers(1, &stagingVBO);
			if (stagingIBO) glDeleteBuffers(1, &stagingIBO);
			if (syncObj) glDeleteSync(syncObj);

			// Reset everything to default
			vao = vbo = ibo = blockID_SSBO = indirectBuffer = distanceToAirSSBO = densitySSBO = stagingVBO = stagingIBO = 0;
			syncObj = nullptr;
			gpuLoaded = false;
		}
	};
	
	extern GLuint _3DNoiseMapPipelineComputeShader;
	extern GLuint _distanceToAirComputeShader;
	extern GLuint _voxelCubesGeometryInitComputeShader;
	extern GLuint _voxelCubesTriangleCounterComputeShader;
	extern GLuint _voxelTerrainPainterComputeShader;
	extern GLuint _voxelCubesSurfaceCullingComputeShader;

	extern GLint _noiseWidthLoc;
	extern GLint _noiseHeightLoc;
	extern GLint _noiseDepthLoc;
	extern GLint _noiseOffsetLoc;
	extern GLint _noiseFrequencyLoc;
	extern GLint _noiseDropoffLoc;

	extern GLint _distanceToAirWidthLoc;
	extern GLint _distanceToAirHeightLoc;
	extern GLint _distanceToAirDepthLoc;

	extern GLint _paintWidthLoc;
	extern GLint _paintHeightLoc;
	extern GLint _paintDepthLoc;

	extern GLint _countWidthLoc;
	extern GLint _countHeightLoc;
	extern GLint _countDepthLoc;

	extern GLint _geomWidthLoc;
	extern GLint _geomHeightLoc;
	extern GLint _geomDepthLoc;
	extern GLint _geomOffsetLoc;
	extern GLint _geomColumnSizeLoc;
	extern GLint _geomRowSizeLoc;

	void Init();
	void InitShaders();
	void InitUniformLocations();
	void Cleanup();

	void InitializeVoxelCubeMesh(VoxelCubeMesh& mesh, int width, int height, int depth);
	void InitializeVoxelCubeMeshSize(VoxelCubeMesh& mesh, int size);
	void CreateFlat3DNoiseMapPipeLine(VoxelCubeMesh& mesh, const int width, const int height, const int depth, const glm::vec3 offset, bool CleanUp, const float frequency, const bool useDropoff);
	void SampleDistanceToAir(VoxelCubeMesh& mesh, int width, int height, int depth);
	void TerrainPaint(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int height, int depth);

	void PerformVoxelCubesSurfaceCulling(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int height, int depth, float isoLevel);
	int VoxelCubesQuadCount(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int heigth, int depth, glm::vec3 offset, bool CleanUp);
	void VoxelCubesGeometryInit(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int heigth, int depth, glm::vec3 offset, int quadCount, bool CleanUp);
	std::unique_ptr<VoxelCubeMesh> CreateVoxelCubes3DMesh(int width, int heigth, int depth, glm::vec2 offset, bool CleanUp, const float amplitude = 1.0f, const float frequency = 1.0f, const float persistance = 0.5f, const float lacunarity = 2.0f, const int octaves = 5, const bool useDropoff = true);

	
}