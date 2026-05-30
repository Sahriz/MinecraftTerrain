#pragma once
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <vector>
#include <memory>
#include <glm.hpp>

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

			glGenBuffers(1, &counterSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, counterSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

			glGenBuffers(1, &dataSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, maxCapacity * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

			// Holds {numGroupsX, 1, 1} written by VoxelCubesQuadCount so VoxelCubesGeometryInit
			// can call glDispatchComputeIndirect without reading activeCount back to the CPU.
			glGenBuffers(1, &dispatchIndirect);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, dispatchIndirect);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, 3 * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
		}

		~AppendBuffer() {
			if (counterSSBO)     glDeleteBuffers(1, &counterSSBO);
			if (dataSSBO)        glDeleteBuffers(1, &dataSSBO);
			if (dispatchIndirect) glDeleteBuffers(1, &dispatchIndirect);
		}

		AppendBuffer(const AppendBuffer&) = delete;
		AppendBuffer& operator=(const AppendBuffer&) = delete;

		AppendBuffer(AppendBuffer&& other) noexcept
			: counterSSBO(other.counterSSBO), dataSSBO(other.dataSSBO),
			  dispatchIndirect(other.dispatchIndirect), maxCapacity(other.maxCapacity) {
			other.counterSSBO     = 0;
			other.dataSSBO        = 0;
			other.dispatchIndirect = 0;
			other.maxCapacity     = 0;
		}

		GLuint getCounterSSBO()      const { return counterSSBO; }
		GLuint getDataSSBO()         const { return dataSSBO; }
		GLuint getDispatchIndirect() const { return dispatchIndirect; }

	private:
		GLuint counterSSBO;
		GLuint dataSSBO;
		GLuint dispatchIndirect;
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
		GLuint vao = 0, ibo = 0;
		GLuint packedData_SSBO = 0;
		// lowResDensity_SSBO holds the stride-2 float density values written by the noise pass.
		// blockID_SSBO holds the full-res uint16_t block IDs written by the interpolation pass.
		GLuint lowResDensity_SSBO = 0, blockID_SSBO = 0, distanceToAirSSBO = 0, indirectBuffer = 0, densitySSBO = 0;
		GLuint ssboVertexCounter = 0;
		GLuint stagingVBO = 0, stagingIBO = 0, stagingIndirect = 0;
		GLsync syncObj = nullptr;

		glm::vec2 offset = glm::vec2(0.0);

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
				Release();

				vao = other.vao;
				ibo = other.ibo;
				packedData_SSBO = other.packedData_SSBO;
				lowResDensity_SSBO = other.lowResDensity_SSBO;
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
				offset = other.offset;

				// CRITICAL: Set 'other' to 0 so its destructor doesn't delete the buffers we just stole
				other.vao = 0;
				other.ibo = 0;
				other.packedData_SSBO = 0;
				other.lowResDensity_SSBO = 0;
				other.blockID_SSBO = 0;
				other.indirectBuffer = 0;
				other.distanceToAirSSBO = 0;
				other.densitySSBO = 0;
				other.ssboVertexCounter = 0;
				other.stagingVBO = 0;
				other.stagingIBO = 0;
				other.stagingIndirect = 0;
				other.syncObj = nullptr;
				other.indexCount = 0;
				other.maxQuards = 0;
				other.gpuLoaded = false;
				other.offset = glm::vec2(0);
			}
			return *this;
		}

		~VoxelCubeMesh() {
			Release();
		}
	private:
		void Release() {
			if (vao) glDeleteVertexArrays(1, &vao);
			if (ibo) glDeleteBuffers(1, &ibo);
			if (packedData_SSBO) glDeleteBuffers(1, &packedData_SSBO);
			if (lowResDensity_SSBO) glDeleteBuffers(1, &lowResDensity_SSBO);
			if (blockID_SSBO) glDeleteBuffers(1, &blockID_SSBO);
			if (indirectBuffer) glDeleteBuffers(1, &indirectBuffer);
			if (distanceToAirSSBO) glDeleteBuffers(1, &distanceToAirSSBO);
			if (densitySSBO) glDeleteBuffers(1, &densitySSBO);
			if (stagingVBO) glDeleteBuffers(1, &stagingVBO);
			if (stagingIBO) glDeleteBuffers(1, &stagingIBO);
			if (ssboVertexCounter) glDeleteBuffers(1, &ssboVertexCounter);
			if (stagingIndirect) glDeleteBuffers(1, &stagingIndirect);
			if (syncObj) glDeleteSync(syncObj);

			vao = ibo = packedData_SSBO = lowResDensity_SSBO = blockID_SSBO = indirectBuffer = distanceToAirSSBO = densitySSBO = stagingVBO = stagingIBO = ssboVertexCounter = stagingIndirect = 0;
			offset = glm::vec2(0);
			syncObj = nullptr;
			gpuLoaded = false;
		}
	};

	// Shader programs
	extern GLuint _lowResNoiseComputeShader;        // Generates stride-2 float density into lowResDensity_SSBO
	extern GLuint _noiseInterpolationComputeShader; // Trilinearly interpolates density into full-res blockID_SSBO
	extern GLuint _distanceToAirComputeShader;
	extern GLuint _voxelCubesGeometryInitComputeShader;
	extern GLuint _voxelCubesTriangleCounterComputeShader;
	extern GLuint _voxelTerrainPainterComputeShader;
	extern GLuint _voxelCubesSurfaceCullingComputeShader;

	// Low-res noise shader uniform locations
	extern GLint _lowResNoise_widthLoc;
	extern GLint _lowResNoise_heightLoc;
	extern GLint _lowResNoise_depthLoc;
	extern GLint _lowResNoise_offsetLoc;
	extern GLint _lowResNoise_frequencyLoc;
	extern GLint _lowResNoise_dropoffLoc;

	// Noise interpolation shader uniform locations
	extern GLint _noiseInterp_widthLoc;
	extern GLint _noiseInterp_heightLoc;
	extern GLint _noiseInterp_depthLoc;

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
	// CreateNoise dispatches the low-res noise shader over (width, height, depth) low-res samples.
	// offset must already be pre-shifted by -1 so sample index 0 aligns to padded position -1.
	void CreateNoise(VoxelCubeMesh& mesh, const int width, const int height, const int depth, const glm::vec3 offset, bool CleanUp, const float frequency, const bool useDropoff);
	// InterpolateNoise reads the low-res density and writes full-res block IDs.
	// width/height/depth are the full padded dimensions (e.g. 18x258x18).
	void InterpolateNoise(VoxelCubeMesh& mesh, const int width, const int height, const int depth);
	void SampleDistanceToAir(VoxelCubeMesh& mesh, int width, int height, int depth);
	void TerrainPaint(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int height, int depth);

	void PerformVoxelCubesSurfaceCulling(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int height, int depth, float isoLevel);
	int VoxelCubesQuadCount(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int heigth, int depth, glm::vec3 offset, bool CleanUp);
	void VoxelCubesGeometryInit(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int heigth, int depth, glm::vec3 offset, int quadCount, bool CleanUp);
	std::unique_ptr<VoxelCubeMesh> CreateVoxelCubes3DMesh(int width, int heigth, int depth, glm::vec2 offset, bool CleanUp, const float amplitude = 1.0f, const float frequency = 1.0f, const float persistance = 0.5f, const float lacunarity = 2.0f, const int octaves = 5, const bool useDropoff = true);


}
