#include "App.h"

App::App() : _renderer(), _chunkMeshManager(), _world(), _chunkCreator() {

}

void App::Run() {


	Core::Init();
	auto previous = Clock::now();
	double lag = 0.0;
	_renderer.InitializeInput(_player);

	// Spawn tick thread
	std::thread tickThread([&]() {
		auto previous = Clock::now();
		double lag = 0.0;

		while (_running) {
			auto current = Clock::now();
			Time elapsed = current - previous;
			previous = current;
			lag += elapsed.count();

			while (lag >= TICK_RATE) {
				{
					PlayerInputState snapshot;
					{
						std::lock_guard<std::mutex> lock(_inputMutex);
						snapshot = _currentInput;

						// Reset volatile inputs like mouse deltas after taking the snapshot
						_currentInput.mouseDeltaX = 0.0f;
						_currentInput.mouseDeltaY = 0.0f;
					}

					_world.SetInputSnapshot(snapshot);
				}
				lag -= TICK_RATE;
			}
		}
		});

	//Game Loop
	// Main render loop
	while (!glfwWindowShouldClose(_renderer.GetWindow())) {
		glm::vec3 pos = _renderer.GetCameraPosition(_player);
		glm::vec2 missingChunkCoord;
		for (int i = 0; i < 35; i++) {
			if (_chunkMeshManager.FindMissingChunk(pos, missingChunkCoord)) {
				std::unique_ptr<Core::VoxelCubeMesh> mesh = _chunkCreator.GenerateChunk(missingChunkCoord);
				_chunkMeshManager.InsertChunk(std::move(mesh), missingChunkCoord);
			}
			else { break; }
		}
		

		_renderer.Render(_chunkMeshManager, _player);
	}

	// Cleanup
	_running = false;
	tickThread.join();
	Core::Cleanup();
	_renderer.Cleanup(_chunkMeshManager);

}

void App::UpdateCursorState() {
	if (_cursorEnabled) {
		glfwSetInputMode(_renderer.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		_cursorEnabled = false;
		_firstMouse = true; // Reset mouse to prevent jumping when re-enabling
	}
	else {
		glfwSetInputMode(_renderer.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		_cursorEnabled = true;
	}
}

void App::HandleKeyboardInput(float deltaTime) {
	GLFWwindow* window = _renderer.GetWindow();

	// 1. Handle Application/OS inputs first
	int newState = glfwGetKey(window, GLFW_KEY_E);
	if (newState == GLFW_PRESS && _oldState == GLFW_RELEASE) {
		UpdateCursorState();
	}
	_oldState = newState;

	if (_cursorEnabled) return; // Halt game input if UI is active

	// 2. Calculate game metrics
	float cameraSpeed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 75.0f : 15.0f;
	float velocity = cameraSpeed * deltaTime;

	// 3. Route to Player (Lock mutex if running multithreaded!)
	std::lock_guard<std::mutex> lock(_inputMutex);

	_currentInput.moveForward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
	_currentInput.moveBackward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
	_currentInput.moveLeft = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
	_currentInput.moveRight = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
	_currentInput.moveUp = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
	_currentInput.moveDown = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
}

void App::ProcessMouseMovement(double xpos, double ypos) {
	if (_cursorEnabled) return;

	if (_firstMouse) {
		_lastX = xpos;
		_lastY = ypos;
		_firstMouse = false;
	}

	float xoffset = xpos - _lastX;
	float yoffset = _lastY - ypos; // reversed: y ranges from bottom to top

	_lastX = xpos;
	_lastY = ypos;

	static float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	// Lock mutex and update player rotation

	std::lock_guard<std::mutex> lock(_inputMutex);
	_currentInput.mouseDeltaX += xoffset; // Accumulate in case of multiple polls
	_currentInput.mouseDeltaY += yoffset;
}