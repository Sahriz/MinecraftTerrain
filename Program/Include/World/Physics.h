#pragma once
#include "Renderer/ChunkMeshManager.h"


class Physics {
public:
    // The only constructor you need
    Physics(ChunkManager& chunkManager) : _chunkManager(chunkManager) {}

    // Delete copying to keep the engine safe
    Physics(const Physics&) = delete;
    Physics& operator=(const Physics&) = delete;

private:
    ChunkManager& _chunkManager;
};