#pragma once
#include "Inventory.h"
#include "Movement.h"
#include "iostream"
#include "Helpers/Transform.h"

enum class PlayerMovement {
	FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN
};

#include "Helpers/InputState.h"

class Player {
public:
	Player() = default;

	/*Player Game loop using tickSystem*/
	void UpdatePlayer(double deltaTime);

	void ApplyInput(const PlayerInputState& input, double deltaTime);

	PlayerTransform GetTransform() const {
		return { _position, _front, _up, _right };
	}

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
	const glm::vec3& GetRight() const { return _right; }

	// Physics state, driven by World::StepPhysics when not in fly mode.
	const glm::vec3& GetVelocity() const { return _velocity; }
	void SetVelocity(const glm::vec3& velocity) { _velocity = velocity; }
	void SetPosition(const glm::vec3& position) { _position = position; }
	bool IsGrounded() const { return _grounded; }
	void SetGrounded(bool grounded) { _grounded = grounded; }

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

	glm::vec3 _velocity{ 0.0f, 0.0f, 0.0f };
	bool _grounded = false;

	float _yaw = -90.0f;
	float _pitch = 0.0f;
};
