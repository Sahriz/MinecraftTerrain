#pragma once
#include <glm.hpp>
#include <functional>

namespace std {
    template <>
    struct hash<glm::vec2> {
        size_t operator()(const glm::vec2& v) const {
            return std::hash<float>()(v.x) ^ (std::hash<float>()(v.y) << 1);
        }
    };
}
