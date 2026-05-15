#include "App.h"

App::App() : _renderer(),_world() {}

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
					std::lock_guard<std::mutex> lock(_worldMutex);
					_world.Tick(lag, _currentInput); // safe update
				}
				lag -= TICK_RATE;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		});

	//Game Loop
	// Main render loop
	while (!glfwWindowShouldClose(_renderer.GetWindow())) {
		glm::vec3 pos = _renderer.GetCameraPosition(_player);
		_chunkManager.Update(pos);

		_renderer.Render(_chunkManager, _player);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	// Cleanup
	_running = false;
	tickThread.join();
	Core::Cleanup();
	_renderer.Cleanup(_chunkManager);

}

void App::PollInput() {
    std::lock_guard<std::mutex> lock(_inputMutex);

    GLFWwindow* window = _renderer.GetWindow(); // Or wherever you store the window pointer

    // --- CURSOR TOGGLE LOGIC ---
    int newState = glfwGetKey(window, GLFW_KEY_E);
    if (newState == GLFW_PRESS && _oldState == GLFW_RELEASE) {
        _cursorEnabled = !_cursorEnabled;
        glfwSetInputMode(window, GLFW_CURSOR, _cursorEnabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
    _oldState = newState;

    // If the cursor is free (e.g., in a menu), stop sending inputs to the game!
    if (_cursorEnabled) {
        _currentInput = InputState{}; // Zero out all inputs so the player stops walking
        _firstMouse = true;           // Reset mouse so camera doesn't jump when we close the menu
        return;
    }

    // --- KEYBOARD INTENT ---
    // Start at zero every frame
    _currentInput.moveForward = 0.0f;
    _currentInput.moveRight = 0.0f;

    // Combine opposite keys (so pressing W and S at the same time equals 0)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) _currentInput.moveForward += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) _currentInput.moveForward -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) _currentInput.moveRight += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) _currentInput.moveRight -= 1.0f;

    _currentInput.isSprinting = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    _currentInput.jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    _currentInput.crouchPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

    // --- MOUSE DELTAS ---
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (_firstMouse) {
        _lastX = xpos;
        _lastY = ypos;
        _firstMouse = false;
    }

    // We only calculate the RAW delta here. 
    // The Player class handles sensitivity, pitch, and yaw math.
    _currentInput.mouseDeltaX = (float)(xpos - _lastX);
    _currentInput.mouseDeltaY = (float)(_lastY - ypos); // Reversed since y-coords go from bottom to top

    _lastX = xpos;
    _lastY = ypos;
}


