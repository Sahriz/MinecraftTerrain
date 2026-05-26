#pragma once
#include <glm.hpp>

struct PlayerTransform {
    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 front{ 0.0f, 0.0f, -1.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 right{ 1.0f, 0.0f, 0.0f };
};
