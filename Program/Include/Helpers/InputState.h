#pragma once

struct PlayerInputState {
    // Movement intent
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;

    // Look intent
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;

    bool isSprinting = false;

    // When true the player free-flies (no gravity/collision); toggled with F.
    bool flyMode = true;

    // Optional: Add actions later (e.g., bool breakBlock = false;)
};
