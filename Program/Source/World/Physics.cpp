#include "World/Physics.h"

#include "World/Player/Player.h"
#include "World/Chunks/ChunkBlockManager.h"
#include "Helpers/InputState.h"

#include <cmath>

void Physics::Step(Player& player, const ChunkBlockManager& blocks,
                   const PlayerInputState& input, float deltaTime) {
    constexpr float kGravity   = 32.0f; // blocks/s^2
    constexpr float kJumpSpeed = 9.0f;  // ~1.25 block jump apex
    constexpr float kWalkSpeed = 5.0f;
    constexpr float kRunSpeed   = 8.0f;
    constexpr float kMaxFall   = 60.0f;

    glm::vec3 pos = player.GetPosition();
    glm::vec3 vel = player.GetVelocity();

    // Horizontal intent from the look vectors flattened onto XZ, so pitch never
    // lifts or sinks the player while walking.
    glm::vec3 flatFront(player.GetFront().x, 0.0f, player.GetFront().z);
    glm::vec3 flatRight(player.GetRight().x, 0.0f, player.GetRight().z);
    if (glm::length(flatFront) > 1e-5f) flatFront = glm::normalize(flatFront);
    if (glm::length(flatRight) > 1e-5f) flatRight = glm::normalize(flatRight);

    glm::vec3 wish(0.0f);
    if (input.moveForward)  wish += flatFront;
    if (input.moveBackward) wish -= flatFront;
    if (input.moveRight)    wish += flatRight;
    if (input.moveLeft)     wish -= flatRight;
    if (glm::length(wish) > 1e-5f) wish = glm::normalize(wish);

    const float speed = input.isSprinting ? kRunSpeed : kWalkSpeed;
    vel.x = wish.x * speed;
    vel.z = wish.z * speed;

    const bool loaded = blocks.IsColumnLoadedAtWorld(pos);
    if (!loaded) {
        // Terrain beneath us hasn't arrived; hold altitude until it streams in.
        vel.y = 0.0f;
    } else {
        if (input.moveUp && player.IsGrounded()) vel.y = kJumpSpeed;
        vel.y -= kGravity * deltaTime;
        if (vel.y < -kMaxFall) vel.y = -kMaxFall;
    }

    const glm::vec3 delta = vel * deltaTime;
    bool grounded = false;

    // Resolve one axis at a time so a blocked axis still lets the others slide.
    // On contact, snap flush against the voxel face and zero that axis' velocity.
    pos.x += delta.x;
    if (BoxBlocked(blocks, pos, 0.05f)) {
        if (delta.x > 0.0f)      pos.x = std::floor(pos.x + kHalfWidth) - kHalfWidth;
        else if (delta.x < 0.0f) pos.x = std::floor(pos.x - kHalfWidth) + 1.0f + kHalfWidth;
        vel.x = 0.0f;
    }

    pos.z += delta.z;
    if (BoxBlocked(blocks, pos, 0.05f)) {
        if (delta.z > 0.0f)      pos.z = std::floor(pos.z + kHalfWidth) - kHalfWidth;
        else if (delta.z < 0.0f) pos.z = std::floor(pos.z - kHalfWidth) + 1.0f + kHalfWidth;
        vel.z = 0.0f;
    }

    pos.y += delta.y;
    if (loaded && BoxBlocked(blocks, pos, 0.0f)) {
        if (delta.y < 0.0f) {
            // Landed: rest the feet on the top face of the voxel below.
            const float feet = pos.y - kEyeHeight;
            pos.y = std::floor(feet) + 1.0f + kEyeHeight;
            grounded = true;
        } else if (delta.y > 0.0f) {
            // Bonked a ceiling: drop the head to the bottom face of the voxel above.
            const float head = pos.y + kHeadAbove;
            pos.y = std::floor(head) - kHeadAbove;
        }
        vel.y = 0.0f;
    }

    player.SetPosition(pos);
    player.SetVelocity(vel);
    player.SetGrounded(grounded);
}

bool Physics::BoxBlocked(const ChunkBlockManager& blocks, const glm::vec3& eyePos, float yOffset) const {
    glm::vec3 bmin(eyePos.x - kHalfWidth, eyePos.y - kEyeHeight + yOffset, eyePos.z - kHalfWidth);
    glm::vec3 bmax(eyePos.x + kHalfWidth, eyePos.y + kHeadAbove, eyePos.z + kHalfWidth);
    return blocks.IsBoxColliding(bmin, bmax);
}
