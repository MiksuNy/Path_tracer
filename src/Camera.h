#pragma once

#include <glm.hpp>

struct Camera
{
public:
	glm::vec3 position, velocity;

	Camera(glm::vec3 position);
};