#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

struct Camera
{
public:
	const float speed = 1.0f;
	bool moving = false;
	
	glm::vec3 position	 = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 forward	 = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 up		 = glm::vec3(0.0, 1.0, 0.0);
	glm::vec3 right		 = glm::vec3(1.0, 0.0, 0.0);
	glm::vec3 target	 = glm::vec3(0.0, 0.0, 0.0);
	glm::mat4 view;

	Camera(glm::vec3 pos);
	void UpdateView(glm::vec3 target);
};