#pragma once

#include "Renderer/Renderer.h"
#include "Chunks/ChunkManager.h"
#include "Player/Player.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

class App {
public:
    App() { }

    void Run();

    ~App() {}

private:
    Renderer _renderer;
    ChunkManager _chunkManager;
    Player _player;
    Physics _physics;

    using Clock = std::chrono::high_resolution_clock;
    using Time = std::chrono::duration<double>;

    const double TICK_RATE = 1.0 / 60.0; // 60 ticks per second

    std::mutex _playerMutex;
    std::atomic<bool> _running{ true }; // This is your shared control flag
};