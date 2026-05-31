#pragma once
#include <glm.hpp>

// Forward declarations only: including Player.h here would close the
// Player -> Movement -> Physics include cycle. The full definitions are pulled
// in by Physics.cpp instead.
class Player;
class ChunkBlockManager;
struct PlayerInputState;

class Physics {
public:
    Physics() = default;

    // Delete copying to keep the engine safe
    Physics(const Physics&) = delete;
    Physics& operator=(const Physics&) = delete;

    // Advance the player one tick: horizontal movement from input, gravity, jump,
    // and per-axis swept AABB collision against the voxel terrain. Vertical motion
    // is frozen while the column under the player has not streamed in yet, so the
    // player never falls through terrain that simply has not arrived from the GPU.
    void Step(Player& player, const ChunkBlockManager& blocks,
              const PlayerInputState& input, float deltaTime);

private:
    // True if the player's AABB (centered on the eye position) overlaps solid terrain.
    bool BoxBlocked(const ChunkBlockManager& blocks, const glm::vec3& eyePos) const;

    // Player collision box: 0.6 wide, 1.8 tall, eye sitting 1.62 above the feet.
    static constexpr float kHalfWidth = 0.3f;
    static constexpr float kEyeHeight = 1.62f;
    static constexpr float kHeadAbove = 0.18f; // 1.8 height - 1.62 eye
};
