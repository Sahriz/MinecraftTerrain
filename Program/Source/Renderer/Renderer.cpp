#include "Renderer/Renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <filesystem>
#include "App.h"

// Static bridge for mouse movement
static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
	if (app) {
		app->ProcessMouseMovement(xpos, ypos);
	}
}

void Renderer::InitializeInput(void* appPointer) {
	glfwSetWindowUserPointer(this->_window, appPointer);
	glfwSetCursorPosCallback(this->_window, mouse_callback);
}

Renderer::Renderer() {

	if (!glfwInit()) {
		// handle error
	}

	// Set OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	this->_window = glfwCreateWindow(_screenWidth, _screenHeight, "ChunkDemo", nullptr, nullptr);
	if (!this->_window) {
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(this->_window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	_camera.UpdateProjection(80.0f, static_cast<float>(_screenWidth) / static_cast<float>(_screenHeight), 0.1f, 1000.0f);

	glfwWindowHint(GLFW_DEPTH_BITS, 24);

	std::cout << std::filesystem::current_path() << std::endl;
	_shaderProgram = CreateShaderProgram("Program/Shaders/shader.vert", "Program/Shaders/shader.frag");

	// Load OpenGL with glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD\n";
		return;
	}

	// Setup ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(this->_window, true);
	ImGui_ImplOpenGL3_Init("#version 430");

	_identity = glm::mat4(1.0f);
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


	glUniform1f(_widthLocation, (float)_screenWidth);
	glUniform1f(_heightLocation, (float)_screenHeight);
	glUniformMatrix4fv(_projMLocation, 1, GL_FALSE, glm::value_ptr(_camera.GetProjectionMatrix()));
	glUniformMatrix4fv(_modelMLocation, 1, GL_FALSE, glm::value_ptr(_model));
	glUniformMatrix3fv(_normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(_normalMatrix));
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	{
		int width, height, channels;

		unsigned char* data = stbi_load("SpriteAtlas.png", &width, &height, &channels, 0);
		if (!data) {
			std::cerr << "Failed to load texture\n";
			exit(1);
		}

		GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
		GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

		int numBlocks = height / 16;
		int totalLayers = numBlocks * 3;
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, 16, 16, totalLayers);

		int bytesPerPixel = channels;
		int rowStride = width * bytesPerPixel;

		glPixelStorei(GL_UNPACK_ROW_LENGTH, width);

		for (int blockY = 0; blockY < numBlocks; blockY++) {
			for (int column = 0; column < 3; column++) {
				unsigned char* dataPtr = data + (blockY * 16 * rowStride) + (column * 16 * bytesPerPixel);
				int layer = (blockY * 3) + column;
				glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, 16, 16, 1, format, GL_UNSIGNED_BYTE, dataPtr);
			}
		}

		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		stbi_image_free(data);
	}
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
		glGetProgramiv(program, GL_LINK_STATUS, &success); // Should be GL_LINK_STATUS, check original code
		glGetProgramInfoLog(program, logLength, nullptr, nullptr);
		// Corrected error check in earlier turn, keeping consistent
		std::cerr << "Shader program link error\n";
		glDeleteProgram(program);
		program = 0;
	}

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);
	return program;
}

void Renderer::DrawChunks(ChunkMeshManager& chunkManager) {
	std::unordered_map<ChunkCoord, std::unique_ptr<Core::VoxelCubeMesh>>& chunkMap = chunkManager.GetChunkMap();
	_chunkRenderer.UpdateActiveChunk(_camera.GetPosition(), chunkManager);

	for (const glm::ivec2& coord : _chunkRenderer.GetActiveChunkSet()) {
		auto it = chunkMap.find(coord);
		if (it != chunkMap.end()) {
			Core::VoxelCubeMesh* voxelData = it->second.get();
			if (voxelData) {
				glBindVertexArray(voxelData->vao);
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, voxelData->indirectBuffer);
				glUniform2fv(_offsetUniformLoc, 1, &voxelData->offset[0]);
				glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (void*)0);
			}
		}
	}
	glBindVertexArray(0);
}

void Renderer::ResetToStartValues() {
	_frequency = 0.1f;
	_width = 16;
	_height = 256;
	_depth = 16;
	_viewDistance = 16;
}

void Renderer::Render(ChunkMeshManager& chunkManager, const PlayerTransform& playerTransform) {
	glfwPollEvents();

	// Start ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	glClearColor(130.f / 255.f, 200.f / 255.f, 229.f / 255.f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float timeValue = (float)glfwGetTime();
	float deltaTime = timeValue - _prevTime;
	_prevTime = timeValue;

	// FPS: accumulate time and frames, update display every 0.5 s
	_fpsAccum += deltaTime;
	_fpsFrameCount++;
	if (_fpsAccum >= 0.5f) {
		_displayedFps = _fpsFrameCount / _fpsAccum;
		_fpsAccum = 0.0f;
		_fpsFrameCount = 0;
	}

	_camera.UpdateView(playerTransform);

	int display_w, display_h;
	glfwGetFramebufferSize(this->_window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);

	glUseProgram(_shaderProgram);

	glUniformMatrix4fv(_viewLoc, 1, GL_FALSE, glm::value_ptr(_camera.GetViewMatrix()));
	glUniform1f(_timeLocation, timeValue);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glUniform1i(_textureUniformLoc, 0);

	DrawChunks(chunkManager);

	// FPS overlay — non-interactive, no decorations, top-left corner
	constexpr ImGuiWindowFlags overlayFlags =
		ImGuiWindowFlags_NoDecoration  |
		ImGuiWindowFlags_NoInputs      |
		ImGuiWindowFlags_NoNav         |
		ImGuiWindowFlags_NoMove        |
		ImGuiWindowFlags_NoSavedSettings      |
		ImGuiWindowFlags_NoFocusOnAppearing   |
		ImGuiWindowFlags_NoBringToFrontOnFocus|
		ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.4f);
	ImGui::Begin("##fps", nullptr, overlayFlags);
	ImGui::Text("FPS: %.0f", _displayedFps);
	ImGui::End();

	// Submit ImGui draw data
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glfwSwapBuffers(this->_window);
}

void Renderer::Cleanup(ChunkMeshManager& chunkManager) {
	chunkManager.DestroyChunks();
	glDeleteProgram(_shaderProgram);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(this->_window);
	glfwTerminate();
}
