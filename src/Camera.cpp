#include "Camera.h"

Camera::Camera(glm::vec3 pos)
{
	position = pos;
}

Camera::Camera()
{
    position = glm::vec3(0.0, 0.0, 0.0);
}

void Camera::UpdateView()
{
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	glm::vec3 worldUp = glm::vec3(0.0, 1.0, 0.0);
	forward = glm::normalize(direction);
	right = glm::normalize(glm::cross(worldUp, forward));
	up = glm::cross(forward, right);

	inverseView = glm::inverse(glm::lookAt(position, position + forward, up));
}

void Camera::ProcessInput(GLFWwindow* window, float dt)
{
    if (glfwGetKey(window, GLFW_KEY_W))
    {
        moving = true;
        position -= forward * speed * dt;
    }
    else if (glfwGetKey(window, GLFW_KEY_S))
    {
        moving = true;
        position += forward * speed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_A))
    {
        moving = true;
        position -= right * speed * dt;
    }
    else if (glfwGetKey(window, GLFW_KEY_D))
    {
        moving = true;
        position += right * speed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_E))
    {
        moving = true;
        position += up * speed * dt;
    }
    else if (glfwGetKey(window, GLFW_KEY_Q))
    {
        moving = true;
        position -= up * speed * dt;
    }
}