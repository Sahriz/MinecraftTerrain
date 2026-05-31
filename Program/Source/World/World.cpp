#include "World/World.h"

void World::Tick(float deltaTime) {
	PlayerInputState input;
	{
		std::lock_guard<std::mutex> lock(_inputMutex);
		input = _inputSnapshot;
	}

	if (input.flyMode) {
		// Free-fly: existing camera-style movement plus look. Reset velocity so that
		// toggling back into physics resumes from rest, not a stale fall.
		_player.ApplyInput(input, deltaTime);
		_player.SetVelocity(glm::vec3(0.0f));
	} else {
		// Walking physics: mouse still looks, but translation comes from Physics::Step.
		_player.Rotate(input.mouseDeltaX, input.mouseDeltaY);
		_physics.Step(_player, _chunkBlockManager, input, deltaTime);
	}
	_player.UpdatePlayer(deltaTime);

	// Pull in any block data the render thread finished reading back, then let
	// UpdateActiveWindow evict whatever is now out of range (authoritative this tick).
	if (_blockDataQueue) {
		_blockDataQueue->Drain([this](glm::vec2 coord, std::vector<uint16_t>&& data) {
			_chunkBlockManager.IngestBlockData(coord, std::move(data));
		});
	}

	_chunkBlockManager.UpdateActiveWindow(_player.GetPosition(), _viewDistance);
}
