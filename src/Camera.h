#pragma once

#include <glm.hpp>

class Camera
{
public:
	glm::vec3 position, velocity, up, right, forward;

	Camera(glm::vec3 position);
};