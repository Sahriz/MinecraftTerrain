#pragma once
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include <GLFW/glfw3.h>
#include "Helpers/InputState.h"

class Camera
{
public:
	Camera(){}

	glm::mat4 GetViewMatrix(const PlayerTransform& transform) const {
		return glm::lookAt(
			transform.position,
			transform.position + transform.forward,
			transform.up
		);
	}

	glm::mat4 GetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane) const {
		return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
	}
};