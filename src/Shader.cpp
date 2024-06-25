#include "Shader.h"

void Shader::Create(GLenum shaderType) { ID = glCreateShader(shaderType); }
void Shader::Parse(const char* filepath)
{
	std::string outString, line;
	std::ifstream inFile(filepath);

	while (getline(inFile, line)) outString += line + "\n";
	inFile.close();

	const char* source = outString.c_str();

	glShaderSource(ID, 1, &source, NULL);
	glCompileShader(ID);
}

Shader::~Shader() { glDeleteShader(ID); }

void Program::Create() { ID = glCreateProgram(); }
void Program::Attach(Shader &shader) { glAttachShader(ID, shader.ID); }
void Program::Link() { glLinkProgram(ID); }
void Program::Use() { glUseProgram(ID); }
void Program::Unuse() { glUseProgram(0); }

void Program::SetUniform1f(const char* uName, float f)
{
	glUniform1f(glGetUniformLocation(ID, uName), f);
}
void Program::SetUniform1i(const char* uName, int i)
{
	glUniform1f(glGetUniformLocation(ID, uName), i);
}
void Program::SetUniform2f(const char* uName, glm::vec2 v)
{
	glUniform2f(glGetUniformLocation(ID, uName), v.x, v.y);
}
void Program::SetUniform3f(const char* uName, glm::vec3 v)
{
	glUniform3f(glGetUniformLocation(ID, uName), v.x, v.y, v.z);
}

void Program::SetUniformCamera(Camera &cam)
{
	this->Use();
	this->SetUniform3f("cam.position", cam.position);
	this->Unuse();
}

Program::~Program() { glDeleteProgram(ID); }