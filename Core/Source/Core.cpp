#include "Core.h"

namespace Core
{
	//Used for functions that are not exposed to the user, but are used internally in the library. Some are CPU implementation of the GPU functions, that are meant for debugging,
	//while others are just utility functions that are used within the library.
	//If for any reason there would be a need to expose these functions, they can be moved to the Core namespace and made public. Just remember to declare them in the
	//header file Core.h.
	namespace {
		std::string readFile(const std::string& filePath) {
			std::ifstream file(filePath);
			std::stringstream buffer;
			if (file) {
				buffer << file.rdbuf();
			}
			else {
				std::cerr << "Failed to open file: " << filePath << "\n";
			}
			return buffer.str();
		}
		GLuint CreateComputeShaderProgram(const std::string& path) {
			GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
			std::string source = readFile(path);
			const char* src = source.c_str();
			glShaderSource(shader, 1, &src, nullptr);
			glCompileShader(shader);

			// Check compilation status
			GLint success;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				GLint logLength;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
				std::vector<char> log(logLength);
				glGetShaderInfoLog(shader, logLength, nullptr, log.data());
				std::cerr << "Compute Shader compilation failed:\n" << log.data() << std::endl;
				glDeleteShader(shader);
				return 0;
			}

			// Link shader into a program
			GLuint program = glCreateProgram();
			glAttachShader(program, shader);
			glLinkProgram(program);

			glGetProgramiv(program, GL_LINK_STATUS, &success);
			if (!success) {
				GLint logLength;
				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
				std::vector<char> log(logLength);
				glGetProgramInfoLog(program, logLength, nullptr, log.data());
				std::cerr << "Program linking failed:\n" << log.data() << std::endl;
				glDeleteShader(shader);
				glDeleteProgram(program);
				return 0;
			}
			glDeleteShader(shader); // Safe to delete after linking
			return program;
		}
	}
	
		GLuint _3DNoiseMapPipelineComputeShader = 0;
		GLuint _distanceToAirComputeShader = 0;
		GLuint _voxelCubesGeometryInitComputeShader = 0;
		GLuint _voxelCubesTriangleCounterComputeShader = 0;
		GLuint _voxelTerrainPainterComputeShader = 0;
		GLuint _voxelCubesSurfaceCullingComputeShader = 0;

		GLint _noiseWidthLoc = 0;
		GLint _noiseHeightLoc = 0;
		GLint _noiseDepthLoc = 0;
		GLint _noiseOffsetLoc = 0;
		GLint _noiseFrequencyLoc = 0;
		GLint _noiseDropoffLoc = 0;

		GLint _distanceToAirWidthLoc = 0;
		GLint _distanceToAirHeightLoc = 0;
		GLint _distanceToAirDepthLoc = 0;

		GLint _paintWidthLoc = 0;
		GLint _paintHeightLoc = 0;
		GLint _paintDepthLoc = 0;

		GLint _countWidthLoc = 0;
		GLint _countHeightLoc = 0;
		GLint _countDepthLoc = 0;

		GLint _geomWidthLoc = 0;
		GLint _geomHeightLoc = 0;
		GLint _geomDepthLoc = 0;
		GLint _geomOffsetLoc = 0;
		GLint _geomColumnSizeLoc = 0;
		GLint _geomRowSizeLoc = 0;
		

		void Init() {
			InitShaders();
			InitUniformLocations();
		}

		void InitShaders() {
			_3DNoiseMapPipelineComputeShader = CreateComputeShaderProgram("Core/Source/3DNoiseExpirimant.comp");
			_distanceToAirComputeShader = CreateComputeShaderProgram("Core/Source/distanceToAir.comp");
			_voxelCubesGeometryInitComputeShader = CreateComputeShaderProgram("Core/Source/GeometryInit.comp");
			_voxelCubesTriangleCounterComputeShader = CreateComputeShaderProgram("Core/Source/CountTriangles.comp");
			_voxelTerrainPainterComputeShader = CreateComputeShaderProgram("Core/Source/TerrainPainter.comp");
			_voxelCubesSurfaceCullingComputeShader = CreateComputeShaderProgram("Core/Source/SurfaceCulling.comp");
		}

		void InitUniformLocations() {

			_noiseWidthLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "width");
			_noiseHeightLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "height");
			_noiseDepthLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "depth");
			_noiseOffsetLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "offset");
			_noiseFrequencyLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "frequency");
			_noiseDropoffLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "useHeightDropoff");

			_distanceToAirWidthLoc = glGetUniformLocation(_distanceToAirComputeShader, "width");
			_distanceToAirHeightLoc = glGetUniformLocation(_distanceToAirComputeShader, "height");
			_distanceToAirDepthLoc = glGetUniformLocation(_distanceToAirComputeShader, "depth");

			_paintWidthLoc = glGetUniformLocation(_voxelTerrainPainterComputeShader, "width");
			_paintHeightLoc = glGetUniformLocation(_voxelTerrainPainterComputeShader, "height");
			_paintDepthLoc = glGetUniformLocation(_voxelTerrainPainterComputeShader, "depth");

			_countWidthLoc = glGetUniformLocation(_voxelCubesTriangleCounterComputeShader, "gridWidth");
			_countHeightLoc = glGetUniformLocation(_voxelCubesTriangleCounterComputeShader, "gridHeigth");
			_countDepthLoc = glGetUniformLocation(_voxelCubesTriangleCounterComputeShader, "gridDepth");

			_geomWidthLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "gridWidth");
			_geomHeightLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "gridHeigth");
			_geomDepthLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "gridDepth");
			_geomOffsetLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "offset");
			_geomColumnSizeLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "columns");
			_geomRowSizeLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "rows");
		}

		void Cleanup() {
			glDeleteProgram(_3DNoiseMapPipelineComputeShader);
			glDeleteProgram(_voxelCubesGeometryInitComputeShader);
			glDeleteProgram(_voxelCubesTriangleCounterComputeShader);
			glDeleteProgram(_voxelTerrainPainterComputeShader);
			glDeleteProgram(_voxelCubesSurfaceCullingComputeShader);
		}

		void InitializeVoxelCubeMesh(VoxelCubeMesh& mesh, int width, int height, int depth) {
			int totalVoxels = width * height * depth;

			glGenBuffers(1, &mesh.indirectBuffer);
			uint32_t drawCmd[] = { 0, 1, 0, 0, 0 };
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, mesh.indirectBuffer);
			glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(drawCmd), drawCmd, GL_DYNAMIC_DRAW);

			glGenVertexArrays(1, &mesh.vao);
			glBindVertexArray(mesh.vao);

			glGenBuffers(1, &mesh.stagingIndirect);
			glBindBuffer(GL_COPY_READ_BUFFER, mesh.stagingIndirect);
			glBufferData(GL_COPY_READ_BUFFER, 16, nullptr, GL_STREAM_READ);

			glGenBuffers(1, &mesh.blockID_SSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh.blockID_SSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, totalVoxels * sizeof(uint16_t), NULL, GL_DYNAMIC_COPY);

			glGenBuffers(1, &mesh.distanceToAirSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh.distanceToAirSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, totalVoxels * sizeof(uint16_t), NULL, GL_DYNAMIC_COPY);
		}

		void InitializeVoxelCubeMeshSize(VoxelCubeMesh& mesh, int size) {
			if (size > -1) {
				mesh.maxQuards = size;

				glGenBuffers(1, &mesh.packedData_SSBO);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh.packedData_SSBO);
				glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * size * sizeof(uint32_t), NULL, GL_DYNAMIC_COPY);

				glGenBuffers(1, &mesh.ibo);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh.ibo);
				glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * size * sizeof(uint16_t), NULL, GL_DYNAMIC_COPY);

				// 3. Setup Vertex Counter (FIX: pass memory address of 0, not literal 0)
				int initialVertex = 0;
				glGenBuffers(1, &mesh.ssboVertexCounter);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh.ssboVertexCounter);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int), &initialVertex, GL_DYNAMIC_COPY);

				glBindVertexArray(mesh.vao);

				glBindBuffer(GL_ARRAY_BUFFER, mesh.packedData_SSBO);

				glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(uint32_t), (void*)0);
				glEnableVertexAttribArray(0);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);

				glBindVertexArray(0);
				mesh.gpuLoaded = true;
			}
		}


		void CreateFlat3DNoiseMapPipeLine(VoxelCubeMesh& mesh, const int width, const int height, const int depth, const glm::vec3 offset, bool CleanUp, const float frequency, const bool useDropoff) {

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.blockID_SSBO);

			glUseProgram(_3DNoiseMapPipelineComputeShader);

			glUniform1i(_noiseWidthLoc, width);
			glUniform1i(_noiseHeightLoc, height);
			glUniform1i(_noiseDepthLoc, depth);
			glUniform3fv(_noiseOffsetLoc, 1, &offset[0]);
			glUniform1f(_noiseFrequencyLoc, frequency);
			glUniform1i(_noiseDropoffLoc, useDropoff);

			glDispatchCompute(
				(GLuint)ceil(width / 8.0f),
				(GLuint)ceil(height / 8.0f),
				(GLuint)ceil(depth / 8.0f)
			);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		void SampleDistanceToAir(VoxelCubeMesh& mesh, int width, int height, int depth) {

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.blockID_SSBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mesh.distanceToAirSSBO);
			

			glUseProgram(_distanceToAirComputeShader);

			glUniform1i(_distanceToAirWidthLoc, width);
			glUniform1i(_distanceToAirHeightLoc, height);
			glUniform1i(_distanceToAirDepthLoc, depth);

			glDispatchCompute(
				(GLuint)ceil(width / 8.0f),
				(GLuint)ceil(height / 8.0f),
				(GLuint)ceil(depth / 8.0f)
			);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}
		void TerrainPaint(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int height, int depth) {
			int maxActiveVoxels = width * height * depth;

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.blockID_SSBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ab.getCounterSSBO());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ab.getDataSSBO());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mesh.distanceToAirSSBO);

			glUseProgram(_voxelTerrainPainterComputeShader);

			glUniform1i(_paintWidthLoc, width);
			glUniform1i(_paintHeightLoc, height);
			glUniform1i(_paintDepthLoc, depth);

			int numGroups = (maxActiveVoxels + 63) / 64;

			glDispatchCompute(numGroups, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		void PerformVoxelCubesSurfaceCulling(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int height, int depth, float isoLevel) {
			// 1. Reset the AppendBuffer counter to 0 so we start fresh for this chunk
			uint32_t zero = 0;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ab.getCounterSSBO());
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &zero);

			// 2. Memory Barrier: Ensure the Noise Map is finished before we read it
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			// 3. Bind the Culling Shader and its buffers
			glUseProgram(_voxelCubesSurfaceCullingComputeShader);

			// Binding 0: The Noise Density (Input)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.blockID_SSBO);
			// Binding 1: The AppendBuffer Counter (Output)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ab.getCounterSSBO());
			// Binding 2: The AppendBuffer Data List (Output)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ab.getDataSSBO());

			// 4. Set Uniforms
			glUniform1i(glGetUniformLocation(_voxelCubesSurfaceCullingComputeShader, "width"), width);
			glUniform1i(glGetUniformLocation(_voxelCubesSurfaceCullingComputeShader, "height"), height);
			glUniform1i(glGetUniformLocation(_voxelCubesSurfaceCullingComputeShader, "depth"), depth);

			// 5. Dispatch: One thread per voxel
			glDispatchCompute((GLuint)ceil(width / 8.0f),
				(GLuint)ceil(height / 8.0f),
				(GLuint)ceil(depth / 8.0f));

			// 6. Memory Barrier: Ensure the Active List is built before the Counting step starts
			glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT |
				GL_ELEMENT_ARRAY_BARRIER_BIT |
				GL_COMMAND_BARRIER_BIT |
				GL_SHADER_STORAGE_BARRIER_BIT);
		}

		int VoxelCubesQuadCount(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int heigth, int depth, glm::vec3 offset, bool CleanUp) {
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.blockID_SSBO);

			GLuint ssboCounter;
			int initial = 0;
			glGenBuffers(1, &ssboCounter);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCounter);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int), &initial, GL_DYNAMIC_COPY);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCounter);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ab.getCounterSSBO());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ab.getDataSSBO());

			glUseProgram(_voxelCubesTriangleCounterComputeShader);
		
			glUniform1i(_countWidthLoc, width);
			glUniform1i(_countHeightLoc, heigth);
			glUniform1i(_countDepthLoc, depth);

			uint32_t activeCount = 0;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ab.getCounterSSBO());
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &activeCount);
			// 2. Only dispatch if there is actually something to draw
			if (activeCount > 0) {
				// We use a 1D dispatch. 
				// Since local_size_x = 64, we divide the total count by 64.
				GLuint numGroups = (activeCount + 63) / 64;
			
				glDispatchCompute(numGroups, 1, 1);
			}
			glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT |
				GL_ELEMENT_ARRAY_BARRIER_BIT |
				GL_COMMAND_BARRIER_BIT |
				GL_SHADER_STORAGE_BARRIER_BIT);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCounter);
			int* ptr = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

			int vertexCount = 0;
			if (ptr) {
				vertexCount = *ptr;
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			}
			glDeleteBuffers(1, &ssboCounter);
			return vertexCount;
		}

		void VoxelCubesGeometryInit(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int heigth, int depth, glm::vec3 offset, int quadCount, bool CleanUp) {

			uint32_t activeCount = 0;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ab.getCounterSSBO());
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &activeCount);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.blockID_SSBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mesh.ibo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mesh.indirectBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mesh.ssboVertexCounter);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ab.getCounterSSBO());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ab.getDataSSBO());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, mesh.packedData_SSBO);

			glUseProgram(_voxelCubesGeometryInitComputeShader);

			glUniform1i(_geomWidthLoc, width);
			glUniform1i(_geomHeightLoc, heigth);
			glUniform1i(_geomDepthLoc, depth);
			glUniform3fv(_geomOffsetLoc, 1, &offset[0]);
			glUniform1f(_geomColumnSizeLoc, 3);
			glUniform1f(_geomRowSizeLoc, 16);

			GLuint numGroups = (activeCount + 63) / 64;
			//std::cout << activeCount << std::endl;
			glDispatchCompute(numGroups, 1, 1);

			glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT |
				GL_ELEMENT_ARRAY_BARRIER_BIT |
				GL_COMMAND_BARRIER_BIT |
				GL_SHADER_STORAGE_BARRIER_BIT);
		}

		std::unique_ptr<VoxelCubeMesh> CreateVoxelCubes3DMesh(int width, int height, int depth, glm::vec2 offset, bool CleanUp, const float amplitude, const float frequency, const float persistance, const float lacunarity, const int octaves, const bool useDropoff) {
			std::unique_ptr<VoxelCubeMesh> cubeMeshData = std::make_unique<VoxelCubeMesh>();
			cubeMeshData->gpuLoaded = true;
			cubeMeshData->offset = offset;
			int paddedWidth = width + 2;
			int paddedHeight = height + 2;
			int paddedDepth = depth + 2;

			glm::vec3 offset3D = glm::vec3(offset.x, 0, offset.y);

			InitializeVoxelCubeMesh(*cubeMeshData, paddedWidth, paddedHeight, paddedDepth);

			CreateFlat3DNoiseMapPipeLine(*cubeMeshData, paddedWidth, paddedHeight, paddedDepth, offset3D, true, frequency, true);

			AppendBuffer ab(paddedWidth, paddedHeight, paddedDepth);

			PerformVoxelCubesSurfaceCulling(*cubeMeshData, ab, paddedWidth, paddedHeight, paddedDepth, 0.0f);

			SampleDistanceToAir(*cubeMeshData, paddedWidth, paddedHeight, paddedDepth);

			TerrainPaint(*cubeMeshData, ab, paddedWidth, paddedHeight, paddedDepth);

			int quadCount = VoxelCubesQuadCount(*cubeMeshData, ab, paddedWidth, paddedHeight, paddedDepth, offset3D, CleanUp);

			InitializeVoxelCubeMeshSize(*cubeMeshData, quadCount);

			VoxelCubesGeometryInit(*cubeMeshData, ab, paddedWidth, paddedHeight, paddedDepth, offset3D, quadCount, CleanUp);
			glDeleteBuffers(1, &cubeMeshData->distanceToAirSSBO);
			glDeleteBuffers(1, &cubeMeshData->stagingVBO);
			glDeleteBuffers(1, &cubeMeshData->stagingIBO);
			glDeleteBuffers(1, &cubeMeshData->ssboVertexCounter);
			glDeleteBuffers(1, &cubeMeshData->stagingIndirect);
			glDeleteBuffers(1, &cubeMeshData->blockID_SSBO);
			cubeMeshData->blockID_SSBO = 0;

			return cubeMeshData;
		}
	}
