#pragma once

#include <glm.hpp>

#include "Utility.h"
#include "Object.h"
#include "Camera.h"

class Shader
{
public:
	GLuint ID;

	void Create(GLenum shaderType);
	void Parse(const char* filepath);

	~Shader();
};

class Program
{
public:
	GLuint ID;

	void Create();
	void Attach(Shader &shader);
	void Link();
	void Use();
	void Unuse();

	void SetUniform1f(const char* uName, float f);
	void SetUniform1i(const char* uName, int i);
	void SetUniform2f(const char* uName, glm::vec2 v);
	void SetUniform3f(const char* uName, glm::vec3 v);

	void SetUniformCamera(Camera &cam);

	~Program();
private:
	Shader vertexShader;
	Shader fragmentShader;
};