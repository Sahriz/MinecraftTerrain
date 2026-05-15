#pragma once
#include "World/Chunks/ChunkManager.h"
#include "glm.hpp"
#include "World/Player/Player.h"


class Physics {
public:
    // The only constructor you need
    Physics() {}

    void SimulateMove(Player& player, ChunkManager& chunkManager, float deltaTime) {
        glm::vec3 desiredMovement = player.GetIntendedVelocity() * (float)deltaTime;

        glm::vec3 currentPos = player.GetCurrentTransform().position;

        //Collission logic would go here//
        
        glm::vec3 finalSafePosition = currentPos + desiredMovement; // (Assuming no walls for now)

        //Update the player's ACTUAL position
        player.SetPosition(finalSafePosition);
    }

    // Delete copying to keep the engine safe
    Physics(const Physics&) = delete;
    Physics& operator=(const Physics&) = delete;

private:

};