#include "Camera.h"

Camera::Camera(glm::vec3 pos)
{
	position = pos;
}

void Camera::UpdateView()
{
	glm::vec3 worldUp = glm::vec3(0.0, 1.0, 0.0);
	forward = glm::normalize(position - target);
	right = glm::normalize(glm::cross(worldUp, forward));
	up = glm::cross(forward, right);

	view = glm::lookAt(position, position + forward, worldUp);
}