#include "App.h"

App::App() : _renderer(), _chunkManager(), _physics(_chunkManager), _player(_renderer.GetWindow(), &_physics) {

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
					std::lock_guard<std::mutex> lock(_playerMutex);
					_player.UpdatePlayer(lag); // safe update
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



