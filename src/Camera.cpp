#include "Camera.h"

Camera::Camera(glm::vec3 _pos, float _fov)
{
	position = _pos;
	up = glm::vec3(0, 1, 0);
	right = glm::vec3(1, 0, 0);
	forward = glm::vec3(0, 0, -1);
	fov = _fov;
}