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
		GLuint _voxelCubesGeometryInitComputeShader = 0;
		GLuint _voxelCubesTriangleCounterComputeShader = 0;
		GLuint _voxelTerrainPainterComputeShader = 0;
		GLuint _voxelCubesSurfaceCullingComputeShader = 0;
		

		void Init() {
			_3DNoiseMapPipelineComputeShader = CreateComputeShaderProgram("Core/Source/3DNoise.comp");
			_voxelCubesGeometryInitComputeShader = CreateComputeShaderProgram("Core/Source/GeometryInit.comp");
			_voxelCubesTriangleCounterComputeShader = CreateComputeShaderProgram("Core/Source/CountTriangles.comp");
			_voxelTerrainPainterComputeShader = CreateComputeShaderProgram("Core/Source/TerrainPainter.comp");
			_voxelCubesSurfaceCullingComputeShader = CreateComputeShaderProgram("Core/Source/SurfaceCulling.comp");
		}

		void Cleanup() {
			glDeleteProgram(_3DNoiseMapPipelineComputeShader);
			glDeleteProgram(_voxelCubesGeometryInitComputeShader);
			glDeleteProgram(_voxelCubesTriangleCounterComputeShader);
			glDeleteProgram(_voxelTerrainPainterComputeShader);
			glDeleteProgram(_voxelCubesSurfaceCullingComputeShader);
		}


		void CreateFlat3DNoiseMapPipeLine(VoxelCubeMesh& mesh, const Spline& spline, const int width, const int height, const int depth, const glm::vec3 offset, bool CleanUp, const float frequency, const bool useDropoff) {
			int sizeOfNoiseMap = width * height * depth;

			glGenBuffers(1, &mesh.blockID_SSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh.blockID_SSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeOfNoiseMap * sizeof(int), NULL, GL_DYNAMIC_COPY);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.blockID_SSBO);

			glGenBuffers(1, &mesh.splineSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh.splineSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, spline.points.size() * sizeof(glm::vec2), spline.points.data(), GL_DYNAMIC_COPY);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mesh.splineSSBO);

			GLint widthLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "width");
			GLint heightLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "height");
			GLint depthLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "depth");
			GLint offsetLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "offset");
			GLint frequencyLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "frequency");
			GLint dropoffLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "useHeightDropoff");
			GLint splinePointsLoc = glGetUniformLocation(_3DNoiseMapPipelineComputeShader, "splinePointsCount");

			glUseProgram(_3DNoiseMapPipelineComputeShader);

			glUniform1i(widthLoc, width);
			glUniform1i(heightLoc, height);
			glUniform1i(depthLoc, depth);
			glUniform3fv(offsetLoc, 1, &offset[0]);
			glUniform1f(frequencyLoc, frequency);
			glUniform1i(dropoffLoc, useDropoff);
			glUniform1i(splinePointsLoc, (int)spline.points.size());

			glDispatchCompute(
				(GLuint)ceil(width / 8.0f),
				(GLuint)ceil(height / 8.0f),
				(GLuint)ceil(depth / 8.0f)
			);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}
		void TerrainPaint(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int height, int depth) {
			int sizeOfNoiseMap = width * height * depth;


			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.blockID_SSBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ab.counterSSBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ab.dataSSBO);


			GLint widthLoc = glGetUniformLocation(_voxelTerrainPainterComputeShader, "width");
			GLint heightLoc = glGetUniformLocation(_voxelTerrainPainterComputeShader, "height");
			GLint depthLoc = glGetUniformLocation(_voxelTerrainPainterComputeShader, "depth");

			glUseProgram(_voxelTerrainPainterComputeShader);

			glUniform1i(widthLoc, width);
			glUniform1i(heightLoc, height);
			glUniform1i(depthLoc, depth);

			glDispatchCompute(
				(GLuint)ceil(width / 8.0f),
				(GLuint)ceil(height / 8.0f),
				(GLuint)ceil(depth / 8.0f)
			);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		
		void SetupAppendBufferVoxelMesh(AppendBuffer& ab, int width, int height, int depth) {
			ab.maxCapacity = width * height * depth;

			// 1. Setup Counter (just 4 bytes)
			glGenBuffers(1, &ab.counterSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ab.counterSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

			// 2. Setup Data List
			glGenBuffers(1, &ab.dataSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ab.dataSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, ab.maxCapacity * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);
		}

		void ClearAndBindAppendBuffer(AppendBuffer& ab) {
			// Reset counter to 0
			uint32_t zero = 0;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ab.counterSSBO);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &zero);

			// Bind to the binding points defined in the shader
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ab.counterSSBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ab.dataSSBO);
		}		

		void PerformVoxelCubesSurfaceCulling(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int height, int depth, float isoLevel) {
			// 1. Reset the AppendBuffer counter to 0 so we start fresh for this chunk
			uint32_t zero = 0;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ab.counterSSBO);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &zero);

			// 2. Memory Barrier: Ensure the Noise Map is finished before we read it
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			// 3. Bind the Culling Shader and its buffers
			glUseProgram(_voxelCubesSurfaceCullingComputeShader);

			// Binding 0: The Noise Density (Input)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.blockID_SSBO);
			// Binding 1: The AppendBuffer Counter (Output)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ab.counterSSBO);
			// Binding 2: The AppendBuffer Data List (Output)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ab.dataSSBO);

			// 4. Set Uniforms
			glUniform1i(glGetUniformLocation(_voxelCubesSurfaceCullingComputeShader, "width"), width);
			glUniform1i(glGetUniformLocation(_voxelCubesSurfaceCullingComputeShader, "height"), height);
			glUniform1i(glGetUniformLocation(_voxelCubesSurfaceCullingComputeShader, "depth"), depth);

			// 5. Dispatch: One thread per voxel
			glDispatchCompute((GLuint)ceil(width / 8.0f),
				(GLuint)ceil(height / 8.0f),
				(GLuint)ceil(depth / 8.0f));

			// 6. Memory Barrier: Ensure the Active List is built before the Counting step starts
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
		}

		int VoxelCubesQuadCount(VoxelCubeMesh& mesh, AppendBuffer& ab, int width, int heigth, int depth, glm::vec3 offset, bool CleanUp) {
		

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.blockID_SSBO);

			GLuint ssboCounter;
			int initial = 0;
			glGenBuffers(1, &ssboCounter);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCounter);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int), &initial, GL_DYNAMIC_COPY);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCounter);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ab.counterSSBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ab.dataSSBO);

			GLint widthLoc = glGetUniformLocation(_voxelCubesTriangleCounterComputeShader, "gridWidth");
			GLint heightLoc = glGetUniformLocation(_voxelCubesTriangleCounterComputeShader, "gridHeigth");
			GLint depthLoc = glGetUniformLocation(_voxelCubesTriangleCounterComputeShader, "gridDepth");

			glUseProgram(_voxelCubesTriangleCounterComputeShader);
		
			glUniform1i(widthLoc, width);
			glUniform1i(heightLoc, heigth);
			glUniform1i(depthLoc, depth);

			uint32_t activeCount = 0;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ab.counterSSBO);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &activeCount);
			// 2. Only dispatch if there is actually something to draw
			if (activeCount > 0) {
				// We use a 1D dispatch. 
				// Since local_size_x = 64, we divide the total count by 64.
				GLuint numGroups = (activeCount + 63) / 64;
			
				glDispatchCompute(numGroups, 1, 1);
			}
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

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
			int gridSize = width * heigth * depth;

			GLuint ssboIndexCounter;
			GLuint ssboVertexCounter;

			if (mesh.vao) { glDeleteVertexArrays(1, &mesh.vao); mesh.vao = 0; }
			if (mesh.vbo) { glDeleteBuffers(1, &mesh.vbo); mesh.vbo = 0; }
			if (mesh.ibo) { glDeleteBuffers(1, &mesh.ibo); mesh.ibo = 0; }

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.blockID_SSBO);

			glGenBuffers(1, &mesh.vbo);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh.vbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * quadCount * sizeof(VoxelCubeCombinedVertex),NULL, GL_DYNAMIC_COPY);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mesh.vbo);

			glGenBuffers(1, &mesh.ibo);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh.ibo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * quadCount * sizeof(int), NULL, GL_DYNAMIC_COPY);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mesh.ibo);

			int initialIndex = 0;
			glGenBuffers(1, &ssboIndexCounter);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboIndexCounter);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int), &initialIndex, GL_DYNAMIC_COPY);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboIndexCounter);

			int initialVertex = 0;
			glGenBuffers(1, &ssboVertexCounter);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVertexCounter);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int), &initialVertex, GL_DYNAMIC_COPY);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboVertexCounter);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ab.counterSSBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ab.dataSSBO);


			GLint widthLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "gridWidth");
			GLint heigthLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "gridHeigth");
			GLint depthLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "gridDepth");
			GLint offsetLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "offset");
			GLint columnSizeLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "columns");
			GLint rowSizeLoc = glGetUniformLocation(_voxelCubesGeometryInitComputeShader, "rows");

			glUseProgram(_voxelCubesGeometryInitComputeShader);

			glUniform1i(widthLoc, width);
			glUniform1i(heigthLoc, heigth);
			glUniform1i(depthLoc, depth);
			glUniform3fv(offsetLoc, 1, &offset[0]);
			glUniform1f(columnSizeLoc, 3);
			glUniform1f(rowSizeLoc, 16);

			uint32_t activeCount = 0;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ab.counterSSBO);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &activeCount);

			// 2. Only dispatch if there is actually something to draw
			if (activeCount > 0) {
				// We use a 1D dispatch. 
				// Since local_size_x = 64, we divide the total count by 64.
				GLuint numGroups = (activeCount + 63) / 64;
				//std::cout << activeCount << std::endl;
				glDispatchCompute(numGroups, 1, 1);
			}
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

			glGenVertexArrays(1, &mesh.vao);
			glBindVertexArray(mesh.vao);

			glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
			// Position (Location 0)

			int stride = 48;
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
			glEnableVertexAttribArray(0);
			// Normal (Location 1)
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)16);
			glEnableVertexAttribArray(1);
			// UV (Location 2)
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)32);
			glEnableVertexAttribArray(2);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo); // Attach indices to VAO

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboIndexCounter);
			int finalIndexCount = 0;
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &finalIndexCount);
			mesh.indexCount = finalIndexCount;

			glBindVertexArray(0);

			glDeleteBuffers(1, &ssboIndexCounter);
			glDeleteBuffers(1, &ssboVertexCounter);

		}

		VoxelCubeMesh* CreateVoxelCubes3DMesh(int width, int height, int depth, glm::vec2 offset, bool CleanUp, const float amplitude, const float frequency, const float persistance, const float lacunarity, const int octaves, const bool useDropoff) {
			VoxelCubeMesh* cubeMeshData = new VoxelCubeMesh();
			cubeMeshData->gpuLoaded = true;
			int paddedWidth = width + 2;
			int paddedHeight = height + 2;
			int paddedDepth = depth + 2;

			glm::vec3 offset3D = glm::vec3(offset.x, 0, offset.y);

			/*Spline spline;
			spline.points.push_back(SplinePoint(0.0f,0.3f));
			spline.points.push_back(SplinePoint(0.1f, 0.3f));
			spline.points.push_back(SplinePoint(0.27f, 0.45f));
			spline.points.push_back(SplinePoint(0.33f, 0.4f));
			spline.points.push_back(SplinePoint(0.35f, 0.1f));
			spline.points.push_back(SplinePoint(0.36f, 0.0f));
			spline.points.push_back(SplinePoint(0.37f, 0.1f));
			spline.points.push_back(SplinePoint(0.39f, 0.4f));
			spline.points.push_back(SplinePoint(0.44f, 0.45f));
			spline.points.push_back(SplinePoint(0.61f, 0.5f));
			spline.points.push_back(SplinePoint(0.64f,1.45f));
			spline.points.push_back(SplinePoint(0.7f, 1.45f));
			spline.points.push_back(SplinePoint(0.73f, 0.2f));
			spline.points.push_back(SplinePoint(0.95f, 0.2f));
			spline.points.push_back(SplinePoint(0.96f, 1.55f));
			spline.points.push_back(SplinePoint(0.99f, 1.55f));
			spline.points.push_back(SplinePoint(1.0f, 0.2f));
			*/
			Spline spline;
			spline.points.push_back(SplinePoint(0.0f, 0.3f));
			spline.points.push_back(SplinePoint(0.1f, 0.3f));
			spline.points.push_back(SplinePoint(0.96f, 0.45f));
			spline.points.push_back(SplinePoint(0.98f, 1.4f));
			spline.points.push_back(SplinePoint(1.0f, 1.45f));

			CreateFlat3DNoiseMapPipeLine(*cubeMeshData, spline, paddedWidth, paddedHeight, paddedDepth, offset3D, true, frequency, true);

			AppendBuffer ab;
			SetupAppendBufferVoxelMesh(ab, paddedWidth, paddedHeight, paddedDepth);

			PerformVoxelCubesSurfaceCulling(*cubeMeshData, ab, paddedWidth, paddedHeight, paddedDepth, 0.0f);

			TerrainPaint(*cubeMeshData, ab, paddedWidth, paddedHeight, paddedDepth);

			int quadCount = VoxelCubesQuadCount(*cubeMeshData, ab, paddedWidth, paddedHeight, paddedDepth, offset3D, CleanUp);

			VoxelCubesGeometryInit(*cubeMeshData, ab, paddedWidth, paddedHeight, paddedDepth, offset3D, quadCount, CleanUp);
			glDeleteBuffers(1, &cubeMeshData->stagingVBO);
			glDeleteBuffers(1, &cubeMeshData->stagingIBO);
			glDeleteBuffers(1, &ab.counterSSBO);
			glDeleteBuffers(1, &ab.dataSSBO);
			glDeleteBuffers(1, &cubeMeshData->splineSSBO);

			return cubeMeshData;
		}
	}
