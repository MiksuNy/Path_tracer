#pragma once

#include <glm.hpp>

struct Camera
{
public:
	glm::vec3 position, velocity, up, right, forward;

	Camera(glm::vec3 position);

	void UpdateView();
};