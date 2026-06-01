#include "App.h"
#include "Helpers/Config.h"

App::App() : _renderer(), _chunkMeshManager(), _world() {
	TICK_RATE = 1.0 / Config::Get().tickRate;
	// Wire the block-readback seam: render thread produces into the queue, world
	// thread consumes from it. Neither side references the other.
	_chunkMeshManager.SetBlockDataSink(&_blockDataQueue);
	_world.SetBlockDataQueue(&_blockDataQueue);
}

App::~App() {
}

void App::Run() {
	Core::Init();
	_renderer.InitializeInput(this);

	// Hide cursor initially
	glfwSetInputMode(_renderer.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
				PlayerInputState snapshot;
				{
					std::lock_guard<std::mutex> lock(_inputMutex);
					snapshot = _currentInput;

					// Reset volatile inputs like mouse deltas after taking the snapshot
					_currentInput.mouseDeltaX = 0.0f;
					_currentInput.mouseDeltaY = 0.0f;
				}

				_world.SetInputSnapshot(snapshot);
				_world.Tick((float)TICK_RATE);
				lag -= TICK_RATE;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Yield
		}
	});

	// Main render loop
	float lastFrame = 0.0f;
	while (!glfwWindowShouldClose(_renderer.GetWindow())) {
		float currentFrame = (float)glfwGetTime();
		float deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		HandleKeyboardInput(deltaTime); 

		std::vector<glm::vec2> activeChunks = _world.GetActiveChunksSnapshot();
		_chunkMeshManager.Update(activeChunks);

		_renderer.Render(_chunkMeshManager, _world.GetPlayerTransform());
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
		_firstMouse = true; 
	}
	else {
		glfwSetInputMode(_renderer.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		_cursorEnabled = true;
	}
}

void App::HandleKeyboardInput(float deltaTime) {
	GLFWwindow* window = _renderer.GetWindow();

	int newState = glfwGetKey(window, GLFW_KEY_E);
	if (newState == GLFW_PRESS && _oldState == GLFW_RELEASE) {
		UpdateCursorState();
	}
	_oldState = newState;

	int newFlyState = glfwGetKey(window, GLFW_KEY_F);
	if (newFlyState == GLFW_PRESS && _oldFlyState == GLFW_RELEASE) {
		_flyModeEnabled = !_flyModeEnabled;
	}
	_oldFlyState = newFlyState;

	if (_cursorEnabled) return;

	std::lock_guard<std::mutex> lock(_inputMutex);
	_currentInput.flyMode = _flyModeEnabled;
	_currentInput.moveForward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
	_currentInput.moveBackward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
	_currentInput.moveLeft = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
	_currentInput.moveRight = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
	_currentInput.moveUp = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
	_currentInput.moveDown = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
	_currentInput.isSprinting = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
}

void App::ProcessMouseMovement(double xpos, double ypos) {
	if (_cursorEnabled) return;

	if (_firstMouse) {
		_lastX = (float)xpos;
		_lastY = (float)ypos;
		_firstMouse = false;
	}

	float xoffset = (float)xpos - _lastX;
	float yoffset = _lastY - (float)ypos; 

	_lastX = (float)xpos;
	_lastY = (float)ypos;

	float sensitivity = Config::Get().playerSensitivity;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	std::lock_guard<std::mutex> lock(_inputMutex);
	_currentInput.mouseDeltaX += xoffset; 
	_currentInput.mouseDeltaY += yoffset;
}

void App::HandleKeyboardInput(float deltaTime, GLFWwindow* window) {
    // Legacy / Internal use
}
