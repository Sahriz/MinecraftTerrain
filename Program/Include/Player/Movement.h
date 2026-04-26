#pragma once
#include "Physics.h"

class Movement {
public:
	Movement(Physics& physics) { _physics = _physics; }

private:
	Physics* _physics;
	float _playerSpeed = 15.0f;
	bool _grounded = false;
};