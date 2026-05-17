#include "App.h"

App::App() : _renderer(), _chunkMeshManager(), _physics(_chunkMeshManager), _player(_renderer.GetWindow(), &_physics) {

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



