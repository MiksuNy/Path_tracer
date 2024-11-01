#pragma once

#include <GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

struct Camera
{
	float speed = 2.0f;
	bool moving = false;

	float pitch = 0.0f;
	float yaw = 0.0f;

    float mouseLastX = 0.0f;
    float mouseLastY = 0.0f;
	
	glm::vec3 position	 = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 forward	 = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 up		 = glm::vec3(0.0, 1.0, 0.0);
	glm::vec3 right		 = glm::vec3(1.0, 0.0, 0.0);
	glm::mat4 inverseView;

	Camera(glm::vec3 pos);
	Camera();
	void UpdateView();
	void ProcessInput(GLFWwindow* window, float dt);
};