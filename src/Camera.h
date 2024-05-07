#pragma once

#include <glm.hpp>

class Camera
{
public:
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 forward;
	float fov;

	Camera(glm::vec3 position, float fov);
};