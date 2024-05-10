#include "Camera.h"

Camera::Camera(glm::vec3 _pos)
{
	position = _pos;
	velocity = glm::vec3(0);
	up = glm::vec3(0, 1, 0);
	right = glm::vec3(1, 0, 0);
	forward = glm::vec3(0, 0, _pos.z - 2.0f);
}