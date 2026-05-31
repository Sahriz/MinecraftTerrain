#include "Renderer/Renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <filesystem>
#include "App.h"

namespace {
	// A view frustum as 6 inward-facing planes. Each plane is (a,b,c,d): a point
	// (x,y,z) is on the inside when a*x + b*y + c*z + d >= 0, and inside the
	// frustum when it is inside all 6 planes at once.
	struct Frustum {
		glm::vec4 planes[6];

		// Conservative box test. Returns false ONLY when the box is fully on the
		// outside of at least one plane (so it definitely cannot be seen). A box
		// straddling an edge is kept, so we never wrongly cull something visible.
		bool IsBoxVisible(const glm::vec3& mn, const glm::vec3& mx) const {
			for (int i = 0; i < 6; ++i) {
				const glm::vec4& p = planes[i];
				// The "positive vertex": the box corner furthest along this
				// plane's normal. If even that corner is outside the plane, the
				// whole box is, so the box cannot intersect the frustum.
				const glm::vec3 pv(
					p.x >= 0.0f ? mx.x : mn.x,
					p.y >= 0.0f ? mx.y : mn.y,
					p.z >= 0.0f ? mx.z : mn.z);
				if (p.x * pv.x + p.y * pv.y + p.z * pv.z + p.w < 0.0f)
					return false;
			}
			return true;
		}
	};

	// Pull the 6 frustum planes out of a combined clip matrix (projection * view
	// * model) with the Gribb-Hartmann method. We only need the sign of the
	// plane test, so the planes are left un-normalized (saves 6 sqrts/frame).
	// glm is column-major, so matrix row i is (m[0][i], m[1][i], m[2][i], m[3][i]).
	Frustum ExtractFrustum(const glm::mat4& m) {
		auto row = [&](int i) { return glm::vec4(m[0][i], m[1][i], m[2][i], m[3][i]); };
		const glm::vec4 r0 = row(0), r1 = row(1), r2 = row(2), r3 = row(3);
		Frustum f;
		f.planes[0] = r3 + r0; // left
		f.planes[1] = r3 - r0; // right
		f.planes[2] = r3 + r1; // bottom
		f.planes[3] = r3 - r1; // top
		f.planes[4] = r3 + r2; // near
		f.planes[5] = r3 - r2; // far
		return f;
	}
}

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
		GLint logLength = 0;
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

void Renderer::DrawChunks(ChunkMeshManager& chunkManager) {
	std::unordered_map<ChunkCoord, ChunkResident>& chunkMap = chunkManager.GetChunkMap();
	_chunkRenderer.UpdateActiveChunk(_camera.GetPosition(), chunkManager);

	// Build the view frustum once per frame from the SAME transform the vertex
	// shader uses (projM * uView * uModel). Chunks whose bounding box falls fully
	// outside it are skipped below. This is the big win: the active set is a full
	// disc around the player, but the camera only ever sees a forward wedge of it.
	const glm::mat4 clip = _camera.GetProjectionMatrix() * _camera.GetViewMatrix() * _model;
	const Frustum frustum = ExtractFrustum(clip);

	// One command list of scratch per pool. (Re)size to the pool count once; after
	// that we only clear() each frame, which keeps the capacity so no reallocation.
	const int poolCount = chunkManager.PoolCount();
	if ((int)_drawCommands.size() != poolCount) {
		_drawCommands.assign(poolCount, {});
	}
	for (int p = 0; p < poolCount; ++p) {
		_drawCommands[p].clear();
	}

	// Bucket every visible chunk into its pool's command list. Each command's
	// baseInstance carries the chunk's slot; the vertex shader uses that
	// (gl_BaseInstanceARB) to look up the chunk's offset, which the pool stores
	// per-slot. So there is no per-frame offset list to keep in sync any more.
	for (const glm::vec2& coord : _chunkRenderer.GetActiveChunkSet()) {
		auto it = chunkMap.find(coord);
		if (it == chunkMap.end()) continue;

		const ChunkResident& res = it->second;
		if (res.poolId < 0 || res.quadCount <= 0) continue; // empty chunk or no slot

		// Conservative chunk AABB in pre-model (worldPos) space: x and z span one
		// chunk out from its world offset, y spans the full build height. If it is
		// fully off-screen, skip it - no command, no offset, no GPU work.
		const glm::vec3 boxMin(res.offset.x, 0.0f, res.offset.y);
		const glm::vec3 boxMax(res.offset.x + (float)_width, (float)_height, res.offset.y + (float)_depth);
		if (!frustum.IsBoxVisible(boxMin, boxMax)) continue;

		const ChunkPool& pool = chunkManager.GetPool(res.poolId);
		DrawElementsIndirectCommand cmd;
		cmd.count         = static_cast<GLuint>(res.quadCount) * 6;
		cmd.instanceCount = 1;
		cmd.firstIndex    = pool.FirstIndex(res.slot);  // index units; chunk-local 0-based indices
		cmd.baseVertex    = pool.BaseVertex(res.slot);  // shift them into this slot's vertex range
		cmd.baseInstance  = static_cast<GLuint>(res.slot); // shader reads chunkOffsets[gl_BaseInstanceARB]
		_drawCommands[res.poolId].push_back(cmd);
	}

	// One glMultiDrawElementsIndirect per non-empty pool: upload this frame's
	// command list, bind the pool's static per-slot offset table at SSBO binding
	// 0, then fire the whole pool in a single GL call.
	for (int p = 0; p < poolCount; ++p) {
		const std::vector<DrawElementsIndirectCommand>& cmds = _drawCommands[p];
		if (cmds.empty()) continue;

		ChunkPool& pool = chunkManager.GetPool(p);

		// Orphan the command buffer first (glBufferData with nullptr) so this
		// upload gets fresh storage and can't stall waiting on last frame's draw
		// still reading the old commands. We keep its full per-slot capacity and
		// fill only the visible prefix. Offsets are NOT uploaded here - they live
		// in the pool's per-slot SSBO, written once when each chunk migrated in.
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, pool.IndirectBuffer());
		glBufferData(GL_DRAW_INDIRECT_BUFFER,
			static_cast<GLsizeiptr>(pool.SlotCount()) * sizeof(DrawElementsIndirectCommand),
			nullptr, GL_DYNAMIC_DRAW);
		glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0,
			static_cast<GLsizeiptr>(cmds.size()) * sizeof(DrawElementsIndirectCommand),
			cmds.data());

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pool.OffsetSSBO());

		glBindVertexArray(pool.Vao());
		glMultiDrawElementsIndirect(
			GL_TRIANGLES,
			GL_UNSIGNED_SHORT,
			nullptr,                                  // commands come from the bound indirect buffer
			static_cast<GLsizei>(cmds.size()),
			0);                                       // tightly packed (stride 0 = sizeof(command))
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
