#pragma once

#include "World/Physics.h"
#include "World/Player/Player.h"

#include "Helpers/InputState.h"
#include "Helpers/BlockDataQueue.h"

#include <mutex>
#include <vector>
#include <cstdint>

#include "World/Chunks/ChunkBlockManager.h"

class World {
public:
	World(){}

	// The render thread pushes GPU->CPU block readbacks here; we drain them in Tick.
	void SetBlockDataQueue(BlockDataQueue* queue) { _blockDataQueue = queue; }

	// Advance the simulation one fixed tick: apply input (fly or walking physics),
	// ingest any block data the render thread read back, then refresh the active
	// chunk window. Defined in World.cpp.
	void Tick(float deltaTime);

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

	BlockDataQueue* _blockDataQueue = nullptr; // owned by App; null until wired

	int _viewDistance = 24;
};
