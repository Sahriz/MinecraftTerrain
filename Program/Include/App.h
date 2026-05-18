#pragma once

#include "Renderer/Renderer.h"
#include "Renderer/ChunkMeshManager.h"
#include "Renderer/ChunkCreator.h"
#include "World/Player/Player.h"
#include "World/Physics.h"
#include "World/World.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

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

    // Optional: Add actions later (e.g., bool breakBlock = false;)
};

class App {
public:
    App();

    void Run();

    ~App() {}

private:

    void UpdateCursorState();
    void HandleKeyboardInput(float deltaTime);
    void ProcessMouseMovement(double xpos, double ypos);

    Renderer _renderer;
    ChunkMeshManager _chunkMeshManager;
    ChunkCreator _chunkCreator;
    World _world;

    PlayerInputState _currentInput;


    using Clock = std::chrono::high_resolution_clock;
    using Time = std::chrono::duration<double>;

    const double TICK_RATE = 1.0 / 60.0; // 60 ticks per second

    std::mutex _playerMutex;
    std::mutex _inputMutex;
    std::atomic<bool> _running{ true }; // This is your shared control flag

    float _lastX, _lastY;
    bool _firstMouse;

    bool _cursorEnabled = false;
    int _oldState = GLFW_RELEASE;

    void HandleKeyboardInput(float deltaTime, GLFWwindow* window);
};