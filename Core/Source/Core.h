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
	struct SplinePoint
	{
		SplinePoint(float x, float y) : position(x, y) {}
		glm::vec2 position; //x, y
	};
	struct Spline
	{
		Spline() = default;
		std::vector<SplinePoint> points; //list of points in the spline
		bool closed = false; //if the spline is closed or not
	};
	

	struct AppendBuffer {
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
	struct VoxelCubeMesh {
		GLuint vao = 0;
		GLuint vbo = 0;      // Combined Vertex/Normal/UV data
		GLuint ibo = 0;      // Indices
		GLuint blockID_SSBO = 0; // The 3D grid of Block IDs
		GLuint indirectBuffer = 0; // For indirect draw calls
		GLuint densitySSBO = 0; // For storing the density values of the voxel grid
		GLuint splineSSBO = 0; // For storing spline data for the pipeline noise map
		

		GLuint stagingVBO = 0;
		GLuint stagingIBO = 0;
		GLsync syncObj = nullptr;
		int indexCount = 0;
		int maxQuards = 0;
		bool gpuLoaded = false;

		~VoxelCubeMesh() {
			if (vao) glDeleteVertexArrays(1, &vao);
			if (vbo) glDeleteBuffers(1, &vbo);
			if (ibo) glDeleteBuffers(1, &ibo);
			if (blockID_SSBO) glDeleteBuffers(1, &blockID_SSBO);
			if (indirectBuffer) glDeleteBuffers(1, &indirectBuffer);
			if (densitySSBO) glDeleteBuffers(1, &densitySSBO);
			if (splineSSBO) glDeleteBuffers(1, &splineSSBO);
			if (stagingVBO) glDeleteBuffers(1, &stagingVBO);
			if (stagingIBO) glDeleteBuffers(1, &stagingIBO);
			if (syncObj) glDeleteSync(syncObj);

			vao = vbo = ibo = blockID_SSBO = indirectBuffer = densitySSBO = splineSSBO = stagingVBO = stagingIBO = 0;
			syncObj = nullptr;
			gpuLoaded = false;
			
		}
	};

	struct BlockIds {
		std::vector<int> IDs;
	};
	struct VoxelData {
		VoxelData(VoxelCubeMesh& meshdata, BlockIds& blockids) { meshData = meshdata; blockIDs = blockids; }
		VoxelCubeMesh meshData;
		BlockIds blockIDs;
	};
	
	extern GLuint _3DNoiseMapPipelineComputeShader;
	extern GLuint _voxelCubesGeometryInitComputeShader;
	extern GLuint _voxelCubesTriangleCounterComputeShader;
	extern GLuint _voxelTerrainPainterComputeShader;
	extern GLuint _voxelCubesSurfaceCullingComputeShader;

	void Init();
	void Cleanup();
	void CreateFlat3DNoiseMapPipeLine(VoxelCubeMesh& mesh, const Spline& spline, const int width, const int height, const int depth, const glm::vec3 offset, bool CleanUp, const float frequency, const bool useDropoff);
	void TerrainPaint(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int height, int depth);

	void SetupAppendBufferVoxelMesh(AppendBuffer& ab, int width, int height, int depth);

	void PerformVoxelCubesSurfaceCulling(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int height, int depth, float isoLevel);
	int VoxelCubesQuadCount(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int heigth, int depth, glm::vec3 offset, bool CleanUp);
	void VoxelCubesGeometryInit(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int heigth, int depth, glm::vec3 offset, int quadCount, bool CleanUp);
	VoxelCubeMesh* CreateVoxelCubes3DMesh(int width, int heigth, int depth, glm::vec2 offset, bool CleanUp, const float amplitude = 1.0f, const float frequency = 1.0f, const float persistance = 0.5f, const float lacunarity = 2.0f, const int octaves = 5, const bool useDropoff = true);
}