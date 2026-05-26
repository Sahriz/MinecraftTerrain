#pragma once

#include "World/Physics.h"
#include "World/Player/Player.h"

#include "Helpers/InputState.h"

#include <mutex>

#include "World/Chunks/ChunkBlockManager.h"

class World {
public:
	World(){}

	void Tick(float deltaTime) {
		PlayerInputState input;
		{
			std::lock_guard<std::mutex> lock(_inputMutex);
			input = _inputSnapshot;
		}
		_player.ApplyInput(input, deltaTime);
		_player.UpdatePlayer(deltaTime);
		
		_chunkBlockManager.UpdateActiveWindow(_player.GetPosition(), _viewDistance);
	}

	void SetInputSnapshot(const PlayerInputState& snapshot) {
		std::lock_guard<std::mutex> lock(_inputMutex);
		_inputSnapshot = snapshot;
	}

	std::vector<glm::vec2> GetActiveChunksSnapshot() {
		return _chunkBlockManager.GetActiveChunksSnapshot();
	}

	PlayerTransform GetPlayerTransform() const {
		return _player.GetTransform();
	}

	Player& GetPlayer() {
		return _player;
	}

private:
	Player _player;
	Physics _physics;
	ChunkBlockManager _chunkBlockManager;
	PlayerInputState _inputSnapshot;
	std::mutex _inputMutex;

	int _viewDistance = 24;
};