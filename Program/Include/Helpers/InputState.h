#include "glm.hpp"

struct InputState {
    float moveForward = 0.0f; // 1.0 for W, -1.0 for S
    float moveRight = 0.0f;   // 1.0 for D, -1.0 for A
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    bool isSprinting = false;
    bool jumpPressed = false;
    bool crouchPressed = false;
};

struct PlayerTransform {
    glm::vec3 position{ 0.0f };
    float pitch{ 0.0f };
    float yaw{ 0.0f };

    glm::vec3 forward{ 0.0f, 0.0f, -1.0f };
    glm::vec3 right{ 1.0f, 0.0f, 0.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
};