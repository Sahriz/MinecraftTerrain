#include "World/Player/Player.h"
#include "Helpers/Config.h"

void Player::UpdatePlayer(double deltaTime) {
	
}


void Player::ApplyInput(const PlayerInputState& input, double deltaTime) {
	float cameraSpeed = input.isSprinting ? Config::Get().playerSprintSpeed : Config::Get().playerSpeed;
	float velocity = cameraSpeed * (float)deltaTime;

	if (input.moveForward)  Move(PlayerMovement::FORWARD, velocity);
	if (input.moveBackward) Move(PlayerMovement::BACKWARD, velocity);
	if (input.moveLeft)     Move(PlayerMovement::LEFT, velocity);
	if (input.moveRight)    Move(PlayerMovement::RIGHT, velocity);
	if (input.moveUp)       Move(PlayerMovement::UP, velocity);
	if (input.moveDown)     Move(PlayerMovement::DOWN, velocity);

	Rotate(input.mouseDeltaX, input.mouseDeltaY);
}
