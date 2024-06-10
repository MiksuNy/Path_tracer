#include "Camera.h"

Camera::Camera(glm::vec3 _pos)
{
	position = _pos;
	velocity = glm::vec3(0);
	up = glm::vec3(0, 1, 0);
	right = glm::vec3(1, 0, 0);
	this->UpdateView();
}

void Camera::UpdateView()
{
	forward = glm::vec3(position.x, position.y, position.z - 1.0f);
}