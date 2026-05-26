#pragma once
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include "Helpers/Transform.h"

class Camera {
public:
    Camera() = default;

    void UpdateProjection(float fovDegrees, float aspectRatio, float nearPlane, float farPlane) {
        _projectionMatrix = glm::perspective(glm::radians(fovDegrees), aspectRatio, nearPlane, farPlane);
    }

    void UpdateView(const PlayerTransform& transform) {
        _viewMatrix = glm::lookAt(transform.position, transform.position + transform.front, transform.up);
        _position = transform.position;
    }

    const glm::mat4& GetViewMatrix() const { return _viewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return _projectionMatrix; }
    const glm::vec3& GetPosition() const { return _position; }

private:
    glm::mat4 _viewMatrix{ 1.0f };
    glm::mat4 _projectionMatrix{ 1.0f };
    glm::vec3 _position{ 0.0f };
};
