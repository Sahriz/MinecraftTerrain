#pragma once
#include "Inventory.h"
#include "Camera.h"
#include "Movement.h"
#include "iostream"

enum class PlayerMovement {
	FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN
};

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

class Player {
public:
	Player() = default;

	/*Player Game loop using tickSystem*/
	void UpdatePlayer(double deltaTime);


	/*Camera Related functionallity related to a specific player*/
	const glm::vec3& GetCameraPosition();

	glm::mat4 GetViewMatrix();

	void UpdateCursorState(GLFWwindow* window);

	void HandleKeyboardInput(float deltaTime, GLFWwindow* window);

	void ProcessMouseMovement(GLFWwindow* window, double xpos, double ypos);

	void Move(PlayerMovement direction, float velocity) {
		if (direction == PlayerMovement::FORWARD)  _position += _front * velocity;
		if (direction == PlayerMovement::BACKWARD) _position -= _front * velocity;
		if (direction == PlayerMovement::LEFT)     _position -= _right * velocity;
		if (direction == PlayerMovement::RIGHT)    _position += _right * velocity;
		if (direction == PlayerMovement::UP)       _position += _up * velocity;
		if (direction == PlayerMovement::DOWN)     _position -= _up * velocity;
	}
	
	void Rotate(float xoffset, float yoffset) {
		_yaw += xoffset;
		_pitch += yoffset;

		// Clamp pitch to avoid gimbal lock
		_pitch = glm::clamp(_pitch, -89.0f, 89.0f);
		UpdateVectors();
	}

	const glm::vec3& GetPosition() const { return _position; }
	const glm::vec3& GetFront() const { return _front; }
	const glm::vec3& GetUp() const { return _up; }

private:
	void UpdateVectors() {
		// Recalculate front vector from yaw and pitch
		glm::vec3 direction;
		direction.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
		direction.y = sin(glm::radians(_pitch));
		direction.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));

		_front = glm::normalize(direction);
		_right = glm::normalize(glm::cross(_front, glm::vec3(0.0f, 1.0f, 0.0f))); // World up is 0,1,0
		_up = glm::normalize(glm::cross(_right, _front));
	}

	glm::vec3 _position{ 0.0f, 0.0f, 0.0f };
	glm::vec3 _front{ 0.0f, 0.0f, -1.0f };
	glm::vec3 _up{ 0.0f, 1.0f, 0.0f };
	glm::vec3 _right{ 1.0f, 0.0f, 0.0f };

	float _yaw = -90.0f;
	float _pitch = 0.0f;
};