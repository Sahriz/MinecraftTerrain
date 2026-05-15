#pragma once

#include "Renderer/Renderer.h"
#include "World/World.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

class App {
public:
    App();

    void Run();

    ~App() {}

private:

    void PollInput();

    Renderer _renderer;
    World _world;

    int _oldState = GLFW_RELEASE; // Starts released
    bool _cursorEnabled = false;  // Assuming game starts with captured cursor
    bool _firstMouse = true;      // Prevents camera jumping on the very first frame
    double _lastX = 400.0;        // Start at center of an 800x600 screen (or 0.0)
    double _lastY = 300.0;

    InputState _currentInput;

    using Clock = std::chrono::high_resolution_clock;
    using Time = std::chrono::duration<double>;

    const double TICK_RATE = 1.0 / 60.0; // 60 ticks per second

    std::atomic<bool> _running{ true }; // This is your shared control flag

    std::mutex _inputMutex;      // Protects _currentInput from data races
    std::mutex _worldMutex;
};