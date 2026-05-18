#pragma once

#include "World/Physics.h"
#include "World/Player/Player.h"

struct PlayerInputState {
	// Movement intent
	bool moveForward = false;
	bool moveBackward = false;
	bool moveLeft = false;
	bool moveRight = false;
	bool moveUp = false;
	bool moveDown = false;

	// Look intent
	float mouseDeltaX = 0.0f;
	float mouseDeltaY = 0.0f;

	// Optional: Add actions later (e.g., bool breakBlock = false;)
};


class World {
public:
	World(){}

	void Tick(float deltaTime, GLFWwindow* window) {
		_player.UpdateCursorState(window);
		_player.UpdatePlayer(deltaTime);
	}

	glm::vec3 const GetPlayerPosition() {
		return _player.GetPosition();
	}

	glm::vec3 const GetPlayerFront() {
		return _player.GetFront();
	}

	glm::vec3 const GetPlayerUp() {
		return _player.GetUp();
	}

private:
	Player _player;
	Physics _physics;
};