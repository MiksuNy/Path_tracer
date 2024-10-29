#pragma once

#include <fstream>
#include <string>

#include <glm.hpp>

#include "Error.h"
#include "Object.h"
#include "Camera.h"

struct Shader
{
	GLuint ID;

	Shader(GLenum shaderType, const char* filepath);
	~Shader();
};

struct ComputeProgram
{
	GLuint ID;

	ComputeProgram(const char* filepath);
	~ComputeProgram();
	
	void Use();
	void Unuse();

	void SetUniform1f(const char* uName, float f);
	void SetUniform1i(const char* uName, int i);
	void SetUniform2f(const char* uName, glm::vec2 v);
	void SetUniform3f(const char* uName, glm::vec3 v);
	void SetUniform4f(const char* uName, glm::vec4 v);

	void SetUniformCamera(Camera& cam);
};

struct ShaderProgram
{
	GLuint ID;

	ShaderProgram(const char* vertexPath, const char* fragmentPath);
	~ShaderProgram();

	void Use();
	void Unuse();

	void SetUniform1f(const char* uName, float f);
	void SetUniform1i(const char* uName, int i);
	void SetUniform2f(const char* uName, glm::vec2 v);
	void SetUniform3f(const char* uName, glm::vec3 v);
	void SetUniform4f(const char* uName, glm::vec4 v);

	void SetUniformCamera(Camera &cam);
};