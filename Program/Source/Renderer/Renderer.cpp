#include "Renderer/Renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <filesystem>

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
	if (cam) {
		cam->ProcessMouseMovement(window, xpos, ypos);
	}
}

Renderer::Renderer() {

	if (!glfwInit()) {
		// handle error
	}

	// Set OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	_window = glfwCreateWindow(_screenWidth, _screenHeight, "ChunkDemo", nullptr, nullptr);
	if (!_window) {
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(_window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	_perspectiveMat = glm::perspective(glm::radians(80.0f), static_cast<float>(_screenWidth) / static_cast<float>(_screenHeight), 0.1f, 1000.0f);



	
	glfwSetCursorPosCallback(_window, mouse_callback);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);

	std::cout << std::filesystem::current_path() << std::endl;
	_shaderProgram = CreateShaderProgram("Program/Shaders/shader.vert", "Program/Shaders/shader.frag");

	// Load OpenGL with glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		// handle error
		std::cerr << "Failed to initialize GLAD\n";
		return;
	}

	// Setup ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(_window, true);
	ImGui_ImplOpenGL3_Init("#version 430");

	_identity = glm::mat4(1.0f);
	_view = glm::translate(_identity, glm::vec3(0.0f, 5.0f, 0.0f));
	_model = glm::translate(_identity, glm::vec3(0.0f, -2.0f, 0.0f));
	_normalMatrix = glm::transpose(glm::inverse(glm::mat3(_model)));

	glUseProgram(_shaderProgram);

	_widthLocation = glGetUniformLocation(_shaderProgram, "Width");
	_heightLocation = glGetUniformLocation(_shaderProgram, "Height");
	_timeLocation = glGetUniformLocation(_shaderProgram, "Time");
	_projMLocation = glGetUniformLocation(_shaderProgram, "projM");
	_modelMLocation = glGetUniformLocation(_shaderProgram, "uModel");
	_viewLoc = glGetUniformLocation(_shaderProgram, "uView");
	_normalMatrixLocation = glGetUniformLocation(_shaderProgram, "normalMatrix");
	_textureUniformLoc = glGetUniformLocation(_shaderProgram, "uTexture");
	_offsetUniformLoc = glGetUniformLocation(_shaderProgram, "offset");


	glUniform1f(_widthLocation, _screenWidth);
	glUniform1f(_heightLocation, _screenHeight);
	glUniformMatrix4fv(_projMLocation, 1, GL_FALSE, glm::value_ptr(_perspectiveMat));
	glUniformMatrix4fv(_modelMLocation, 1, GL_FALSE, glm::value_ptr(_model));
	glUniformMatrix4fv(_viewLoc, 1, GL_FALSE, glm::value_ptr(_view));
	glUniformMatrix3fv(_normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(_normalMatrix));
	


	// Main loop

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	{
		

		int width, height, channels;

		unsigned char* data = stbi_load("TerrainLibSpriteMap.png", &width, &height, &channels, 0);
		if (!data) {
			std::cerr << "Failed to load texture\n";
			exit(1);
		}

		// 1. Determine the format based on the image channels
		GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
		GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;

		// 2. Generate and Bind the Texture Array
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

		// 3. Allocate Immutable Storage (The "Container")
		// We need (height / 16) * 3 layers. 
		int numBlocks = height / 16;
		int totalLayers = numBlocks * 3;
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, 16, 16, totalLayers);

		// 4. The Slicer Loop
		int bytesPerPixel = channels;
		int rowStride = width * bytesPerPixel;

		glPixelStorei(GL_UNPACK_ROW_LENGTH, width); // Tell GL how wide the master sheet is

		for (int blockY = 0; blockY < numBlocks; blockY++) {
			for (int column = 0; column < 3; column++) {
				// Calculate the byte offset to the top-left pixel of this 16x16 square
				unsigned char* dataPtr = data + (blockY * 16 * rowStride) + (column * 16 * bytesPerPixel);

				int layer = (blockY * 3) + column;

				// Upload this 16x16 slice to its specific layer
				glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, 16, 16, 1, format, GL_UNSIGNED_BYTE, dataPtr);
			}
		}

		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // Reset to default

		// 5. Setup Parameters (Nearest Filtering for Pixel Art)
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		stbi_image_free(data);
	}

	//Init();
}

void Renderer::InitializeInput(Player& player) {
	glfwSetWindowUserPointer(_window, &player);
}

void Renderer::Init() {
	
}

std::string Renderer::ReadFile(const std::string& filePath) {
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


// Shader loading helpers
GLuint Renderer::CompileShader(GLenum type, const std::string& source) {
	GLuint shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		std::vector<char> log(logLength);
		glGetShaderInfoLog(shader, logLength, nullptr, log.data());
		std::cerr << "Shader compile error:\n" << log.data() << "\n";
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint Renderer::CreateShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
	std::string vertexSource = ReadFile(vertexPath);
	std::string fragmentSource = ReadFile(fragmentPath);

	GLuint vertShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
	if (vertShader == 0) return 0;

	GLuint fragShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
	if (fragShader == 0) {
		glDeleteShader(vertShader);
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		std::vector<char> log(logLength);
		glGetProgramInfoLog(program, logLength, nullptr, log.data());
		std::cerr << "Shader program link error:\n" << log.data() << "\n";
		glDeleteProgram(program);
		program = 0;
	}

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);
	return program;
}

void Renderer::DrawChunks(ChunkMeshManager& chunkManager, Player& player) {
	std::unordered_map<ChunkCoord, std::unique_ptr<Core::VoxelCubeMesh>>& chunkMap = chunkManager.GetChunkMap();
	_chunkRenderer.UpdateActiveChunk(GetCameraPosition(player), chunkManager);

	for (const glm::ivec2& coord : _chunkRenderer.GetActiveChunkSet()) {
		auto it = chunkMap.find(coord);
		if (it != chunkMap.end()) {
			Core::VoxelCubeMesh* voxelData = it->second.get();
			if (voxelData) {
				glBindVertexArray(voxelData->vao);

				// Mandatory for Indirect: Bind the buffer to the INDIRECT_BUFFER target
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, voxelData->indirectBuffer);
				glUniform2fv(_offsetUniformLoc, 1, &voxelData->offset[0]);
				glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (void*)0);
			}
		}

		
	}

	glBindVertexArray(0); // Clean up at the very end
}

void Renderer::ResetToStartValues() {
	_frequency = 0.1f;
	_width = 16;
	_height = 256;
	_depth = 16;
	_viewDistance = 16;
}



void Renderer::Render(ChunkMeshManager& chunkManager, Player& player) {
	glfwPollEvents();
	// 1. Clear the screen
	glClearColor(130.f / 255.f, 200.f / 255.f, 229.f / 255.f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float timeValue = glfwGetTime();
	float deltaTime = timeValue - _prevTime;
	_prevTime = timeValue;

	player.HandleKeyboardInput(deltaTime, _window);
	_view = player.GetViewMatrix();
	
	int display_w, display_h;
	glfwGetFramebufferSize(_window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	

	// 2. BIND THE SHADER FIRST
	glUseProgram(_shaderProgram);

	// 3. NOW UPDATE UNIFORMS (View matrix, time, textures)
	glUniformMatrix4fv(_viewLoc, 1, GL_FALSE, glm::value_ptr(_view));
	glUniform1f(_timeLocation, timeValue);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glUniform1i(_textureUniformLoc, 0);

	// 4. Draw the chunks
	DrawChunks(chunkManager, player);

	glfwSwapBuffers(_window);
}

void Renderer::Cleanup(ChunkMeshManager& chunkManager) {
	chunkManager.DestroyChunks();
	glDeleteProgram(_shaderProgram);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(_window);
	glfwTerminate();
}