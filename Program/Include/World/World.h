#pragma once

#include "World/Chunks/Chunkmanager.h"
#include "World/Player/Player.h"
#include "World/Physics.h"
#include "Helpers/InputState.h"



class World {
public:
	World() : _player(), _physics(), _chunkManager(){}

	void Tick(double deltaTime, const InputState& input) {
		_player.ArchiveState();

		_player.ApplyInput(input, deltaTime);

		_physics.SimulateMove(_player, _chunkManager, deltaTime);

		_chunkManager.Update(_player.GetCurrentPosition());
	}

	

private:
	Player _player;
	Physics _physics;
	ChunkManager _chunkManager;
};