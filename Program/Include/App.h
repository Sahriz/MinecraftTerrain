#pragma once

#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

#include "Helpers/InputState.h"
#include "Helpers/BlockDataQueue.h"
#include "Renderer/Renderer.h"
#include "Renderer/Camera.h"
#include "Renderer/ChunkMeshManager.h"
#include "World/World.h"

class App {
public:
    App();
    ~App();

    void Run();

    // Callbacks for GLFW
    void HandleKeyboardInput(float deltaTime);
    void ProcessMouseMovement(double xpos, double ypos);

private:
    void UpdateCursorState();

    Renderer _renderer;
    ChunkMeshManager _chunkMeshManager;
    World _world;

    // Hands GPU->CPU block readbacks from the render thread to the world thread.
    // Owned here so both _chunkMeshManager (producer) and _world (consumer) outlive it.
    BlockDataQueue _blockDataQueue;

    PlayerInputState _currentInput;

    using Clock = std::chrono::high_resolution_clock;
    using Time = std::chrono::duration<double>;

    double TICK_RATE = 1.0 / 60.0;

    std::mutex _inputMutex;
    std::atomic<bool> _running{ true };

    float _lastX = 0.0f, _lastY = 0.0f;
    bool _firstMouse = true;

    bool _cursorEnabled = false;
    int _oldState = 0; // GLFW_RELEASE

    // Fly-mode toggle (F). Starts enabled so the world opens in free-fly; toggle
    // off to drop into walking physics. Render-thread-only state.
    bool _flyModeEnabled = true;
    int _oldFlyState = 0; // GLFW_RELEASE

    void HandleKeyboardInput(float deltaTime, GLFWwindow* window);
};
