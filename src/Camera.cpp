#include "Camera.h"

Camera::Camera(glm::vec3 _pos)
{
	position = _pos;
	velocity = glm::vec3(0);
}