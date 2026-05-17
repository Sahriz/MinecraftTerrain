#pragma once
#include "Inventory.h"
#include "Camera.h"
#include "Movement.h"
#include "iostream"


class Player {
public:
	Player(GLFWwindow* window, Physics* physics) : _camera(window), _inventory() { _physics = physics; }

	/*Player Game loop using tickSystem*/
	void UpdatePlayer(double deltaTime);


	/*Camera Related functionallity related to a specific player*/
	const glm::vec3& GetCameraPosition();

	glm::mat4 GetViewMatrix();

	void UpdateCursorState(GLFWwindow* window);

	void HandleKeyboardInput(float deltaTime, GLFWwindow* window);

	void ProcessMouseMovement(GLFWwindow* window, double xpos, double ypos);

	

private:
	Camera _camera;
	Inventory _inventory;
	Physics* _physics;
};