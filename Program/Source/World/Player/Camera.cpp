#include "World/Player/Camera.h"

glm::mat4 Camera::GetViewMatrix()
{
	return glm::lookAt(_cameraPos, _cameraPos + _cameraFront, _cameraUp);
}

const glm::vec3& Camera::GetPosition() {
	return _cameraPos;
}

void Camera::UpdateCursorState(GLFWwindow* window) {
	if (_cursorEnabled) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		_cursorEnabled = false;
	}
	else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		_cursorEnabled = true;
	}
}

void Camera::ProcessMouseMovement(GLFWwindow* window, double xpos, double ypos)
{
	if(_cursorEnabled) {
		return; // Ignore mouse movement if cursor is not enabled
	}
	static float sensitivity = 0.1f;

	if (_firstMouse) {
		_lastX = xpos;
		_lastY = ypos;
		_firstMouse = false;
	}

	float xoffset = xpos - _lastX;
	float yoffset = _lastY - ypos; // reversed since y-coordinates range from bottom to top
	_lastX = xpos;
	_lastY = ypos;

	xoffset *= sensitivity;
	yoffset *= sensitivity;

	_yaw += xoffset;
	_pitch += yoffset;

	// clamp _pitch to avoid gimbal lock
	_pitch = glm::clamp(_pitch, -89.0f, 89.0f);

	// recalculate _cameraFront
	glm::vec3 direction;
	direction.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	direction.y = sin(glm::radians(_pitch));
	direction.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	_cameraFront = glm::normalize(direction);
}

