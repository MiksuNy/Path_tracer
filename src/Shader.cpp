#include "Shader.h"

Shader::Shader(GLenum shaderType, const char* filepath)
{
	ID = glCreateShader(shaderType);

	std::string outString, line;
	std::ifstream inFile(filepath);

	while (getline(inFile, line)) outString += line + "\n";
	inFile.close();

	const char* source = outString.c_str();

	glShaderSource(ID, 1, &source, NULL);
	glCompileShader(ID);

	outString.~basic_string();
	line.~basic_string();
}

Shader::~Shader() { glDeleteShader(ID); }



ComputeProgram::ComputeProgram(const char* filepath)
{
	GLuint shaderID = glCreateShader(GL_COMPUTE_SHADER);

	std::string outString, line;
	std::ifstream inFile(filepath);

	while (getline(inFile, line)) outString += line + "\n";
	inFile.close();

	const char* source = outString.c_str();

	glShaderSource(shaderID, 1, &source, NULL);
	glCompileShader(shaderID);

	ID = glCreateProgram();
	glAttachShader(ID, shaderID);
	glLinkProgram(ID);

	glDeleteShader(shaderID);
}

void ComputeProgram::Use() { glUseProgram(ID); }
void ComputeProgram::Unuse() { glUseProgram(ID); }

void ComputeProgram::SetUniform1f(const char* uName, float f)
{
	glUniform1f(glGetUniformLocation(ID, uName), f);
}
void ComputeProgram::SetUniform1i(const char* uName, int i)
{
	glUniform1i(glGetUniformLocation(ID, uName), i);
}
void ComputeProgram::SetUniform2f(const char* uName, glm::vec2 v)
{
	glUniform2f(glGetUniformLocation(ID, uName), v.x, v.y);
}
void ComputeProgram::SetUniform3f(const char* uName, glm::vec3 v)
{
	glUniform3f(glGetUniformLocation(ID, uName), v.x, v.y, v.z);
}
void ComputeProgram::SetUniform4f(const char* uName, glm::vec4 v)
{
	glUniform4f(glGetUniformLocation(ID, uName), v.x, v.y, v.z, v.w);
}

void ComputeProgram::SetUniformCamera(Camera& cam)
{
	SetUniform3f("cam.position", cam.position);
	glUniformMatrix4fv(glGetUniformLocation(this->ID, "cam.inverseView"), 1, GL_FALSE, &cam.inverseView[0][0]);
}

ComputeProgram::~ComputeProgram() { glDeleteProgram(ID); }



ShaderProgram::ShaderProgram(const char* vertexPath, const char* fragmentPath)
{
	Shader vertex(GL_VERTEX_SHADER, vertexPath);
	Shader fragment(GL_FRAGMENT_SHADER, fragmentPath);

	ID = glCreateProgram();
	glAttachShader(ID, vertex.ID);
	glAttachShader(ID, fragment.ID);
	glLinkProgram(ID);

	vertex.~Shader();
	fragment.~Shader();
}
void ShaderProgram::Use() { glUseProgram(ID); }
void ShaderProgram::Unuse() { glUseProgram(0); }

void ShaderProgram::SetUniform1f(const char* uName, float f)
{
	glUniform1f(glGetUniformLocation(ID, uName), f);
}
void ShaderProgram::SetUniform1i(const char* uName, int i)
{
	glUniform1i(glGetUniformLocation(ID, uName), i);
}
void ShaderProgram::SetUniform2f(const char* uName, glm::vec2 v)
{
	glUniform2f(glGetUniformLocation(ID, uName), v.x, v.y);
}
void ShaderProgram::SetUniform3f(const char* uName, glm::vec3 v)
{
	glUniform3f(glGetUniformLocation(ID, uName), v.x, v.y, v.z);
}
void ShaderProgram::SetUniform4f(const char* uName, glm::vec4 v)
{
	glUniform4f(glGetUniformLocation(ID, uName), v.x, v.y, v.z, v.w);
}

void ShaderProgram::SetUniformCamera(Camera &cam)
{
	SetUniform3f("cam.position", cam.position);
	glUniformMatrix4fv(glGetUniformLocation(this->ID, "cam.inverseView"), 1, GL_FALSE, &cam.inverseView[0][0]);
}

ShaderProgram::~ShaderProgram() { glDeleteProgram(ID); }