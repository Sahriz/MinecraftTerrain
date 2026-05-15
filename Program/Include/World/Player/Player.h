#pragma once
#include "iostream"
#include "glm.hpp"
#include "Helpers/InputState.h"


class Player {
public:
	Player(){ }

	/*Player Game loop using tickSystem*/
	void UpdatePlayer(double deltaTime);

	void ApplyInput(const InputState& input, double deltaTime) {
		// 1. Define a sensitivity multiplier (can be a member variable later)
		float mouseSensitivity = 0.1f;
		float walkSpeed = input.isSprinting ? 15.0f : 5.0f;

		// 2. Calculate new Pitch and Yaw
		_currentTransform.yaw += (input.mouseDeltaX * mouseSensitivity);
		_currentTransform.pitch += (input.mouseDeltaY * mouseSensitivity);

		// 3. Clamp the Pitch to prevent gimbal lock
		// We restrict looking past straight up (89.0) and straight down (-89.0)
		_currentTransform.pitch = glm::clamp(_currentTransform.pitch, -89.0f, 89.0f);

		UpdateDirectionVectors();

		glm::vec3 flatForward = glm::normalize(glm::vec3(_currentTransform.forward.x, 0.0f, _currentTransform.forward.z));

		glm::vec3 velocity = glm::vec3(0.0f);
		velocity += flatForward * input.moveForward;
		velocity += _currentTransform.right * input.moveRight;

		if(input.crouchPressed){
			velocity.y -= walkSpeed;
		}

		if (glm::length(velocity) > 0.0f) {
			velocity = glm::normalize(velocity) * walkSpeed;
		}


		glm::vec3 newPosition = GetPreviousPosition() + (velocity * (float)deltaTime);
	}

	void ArchiveState() {
		_previousTransform = _currentTransform;
	}
	
	void UpdateDirectionVectors() {
		glm::vec3 front;
		front.x = cos(glm::radians(_currentTransform.yaw)) * cos(glm::radians(_currentTransform.pitch));
		front.y = sin(glm::radians(_currentTransform.pitch));
		front.z = sin(glm::radians(_currentTransform.yaw)) * cos(glm::radians(_currentTransform.pitch));

		_currentTransform.forward = glm::normalize(front);
		_currentTransform.right = glm::normalize(glm::cross(_currentTransform.forward, glm::vec3(0.0f, 1.0f, 0.0f)));
		_currentTransform.up = glm::normalize(glm::cross(_currentTransform.right, _currentTransform.forward));
	}

	/*Camera Related functionallity related to a specific player*/
	const glm::vec3& GetCameraPosition();

	glm::mat4 GetViewMatrix();

	void UpdateCursorState(GLFWwindow* window);

	void HandleKeyboardInput(float deltaTime, GLFWwindow* window);

	void ProcessMouseMovement(GLFWwindow* window, double xpos, double ypos);

	void SetPosition(glm::vec3 pos) {
		_currentTransform.position = pos;
	}

	glm::vec3 GetCurrentPosition() const {
		return _currentTransform.position;
	}

	glm::vec3 GetPreviousPosition() const {
		return _previousTransform.position;
	}

	PlayerTransform GetCurrentTransform() {
		return _currentTransform;
	}

	PlayerTransform GetPreviousTransform() {
		return _previousTransform;
	}

	glm::vec3 GetIntendedVelocity() {
		return _intendedVelocity;
	}
	

private:
	PlayerTransform _previousTransform;
	PlayerTransform _currentTransform;

	glm::vec3 _intendedVelocity;
};