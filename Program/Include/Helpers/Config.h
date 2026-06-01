#pragma once
#include <string>

struct Config {
    // World
    int viewDistance = 24;
    int tickRate = 60;

    // Chunk
    int chunkWidth = 16;
    int chunkHeight = 256;
    int chunkDepth = 16;

    // Player
    float playerSpeed = 15.0f;
    float playerSprintSpeed = 75.0f;
    float playerSensitivity = 0.1f;

    // Noise
    int noiseOctaves = 5;
    float noiseFrequency = 0.003f;
    float noiseLacunarity = 2.0f;
    float noisePersistence = 0.5f;
    float noiseAmplitude = 1.0f;

    static Config& Get() {
        static Config instance;
        return instance;
    }

    bool Load(const std::string& path);
};
