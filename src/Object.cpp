#include "Object.h"

Sphere::Sphere(struct Program& program, const char* name, glm::vec3 pos, float rad, struct Material& material)
{
    position = pos;
    radius = rad;

	program.Use();
	program.SetUniform3f(std::string(name).append(".position").c_str(),						this->position);
	program.SetUniform1f(std::string(name).append(".radius").c_str(),						this->radius);
	program.SetUniform4f(std::string(name).append(".material.baseColor").c_str(), material.baseColor);
	program.SetUniform4f(std::string(name).append(".material.specularColor").c_str(), material.specularColor);
	program.SetUniform4f(std::string(name).append(".material.emissionColor").c_str(), material.emissionColor);
	program.SetUniform1f(std::string(name).append(".material.smoothness").c_str(), material.smoothness);
	program.SetUniform1f(std::string(name).append(".material.specularSmoothness").c_str(), material.specularSmoothness);
	program.SetUniform1f(std::string(name).append(".material.emissionStrength").c_str(), material.emissionStrength);
	program.SetUniform1f(std::string(name).append(".material.ior").c_str(), material.ior);
	program.SetUniform1f(std::string(name).append(".material.refractionAmount").c_str(), material.refractionAmount);
	program.SetUniform1f(std::string(name).append(".material.specularChance").c_str(), material.specularChance);
	program.Unuse();
}

Triangle::Triangle()
{
	p1.x = 0.0f; p1.y = 0.0f; p1.z = 0.0f; p1.w = 0.0f;
	p2.x = 0.0f; p2.y = 0.0f; p2.z = 0.0f; p1.w = 0.0f;
	p3.x = 0.0f; p3.y = 0.0f; p3.z = 0.0f; p1.w = 0.0f;
}

Triangle::Triangle(struct Program& program, const char* name, glm::vec3 _p1, glm::vec3 _p2, glm::vec3 _p3, uint32_t materialIndex)
{
    p1.x = _p1.x; p1.y = _p1.y; p1.z = _p1.z; p1.w = 0.0f;
	p2.x = _p2.x; p2.y = _p2.y; p2.z = _p2.z; p2.w = 0.0f;
	p3.x = _p3.x; p3.y = _p3.y; p3.z = _p3.z; p3.w = 0.0f;

	program.Use();
	program.SetUniform4f(std::string(name).append(".p1").c_str(), this->p1);
	program.SetUniform4f(std::string(name).append(".p2").c_str(), this->p2);
	program.SetUniform4f(std::string(name).append(".p3").c_str(), this->p3);
	glUniform1ui(glGetUniformLocation(program.ID, std::string(name).append(".materialIndex").c_str()), materialIndex);
	program.Unuse();
}

glm::vec3 Triangle::Center()
{
	float cX = (p1.x + p2.x + p3.x) / 3;
	float cY = (p1.y + p2.y + p3.y) / 3;
	float cZ = (p1.z + p2.z + p3.z) / 3;
	return glm::vec3(cX, cY, cZ);
}

Mesh::Mesh(Scene scene, const char* filePath, uint32_t materialIndex)
{
	this->materialIndex = materialIndex;
	Load(filePath);
	GenBoundingBox();

	GLuint meshSSBO, bvhSSBO, materialSSBO;

	glGenBuffers(1, &meshSSBO);
	glGenBuffers(1, &bvhSSBO);
	glGenBuffers(1, &materialSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, this->tris.size() * sizeof(Triangle), this->tris.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, this->nodes.size() * sizeof(Node), this->nodes.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bvhSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, scene.materials.size() * sizeof(Material), scene.materials.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, materialSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Mesh::Load(const char* filePath)
{
	double timeBeforeLoad = glfwGetTime();

	std::ifstream inFile(filePath);
	std::string line;

	if (!inFile.is_open())
	{
		std::cerr << "File not found: " << filePath << std::endl;
	}
	while (getline(inFile, line))
	{
		if (line.substr(0, 2) == "v ")
		{
			glm::vec4 vertex;
			sscanf_s(line.c_str(), "v %f %f %f", &vertex.x, &vertex.y, &vertex.z); // vertices
			vertex.w = 0.0f;

			vertices.push_back(vertex);
		}
		else if (line.substr(0, 2) == "f ") // indices
		{
			glm::ivec4 index;
			sscanf_s(line.c_str(), "f %i %i %i", &index.x, &index.y, &index.z);
			index.w = 0;

			index.x -= 1;
			index.y -= 1;
			index.z -= 1;

			indices.push_back(index);
		}
	}

	inFile.close();

	for (int i = 0; i < indices.size(); ++i)
	{
		Triangle tempTri;
		tempTri.p1 = vertices[indices[i].x];
		tempTri.p2 = vertices[indices[i].y];
		tempTri.p3 = vertices[indices[i].z];
		tempTri.materialIndex = this->materialIndex;
		tris.push_back(tempTri);
	}

	double timeAfterLoad = glfwGetTime();
	std::cout << "\n\n\n\t'" << filePath << "' took " << timeAfterLoad - timeBeforeLoad << " seconds to load" << "\n";
	std::cout << "\t'" << filePath << "' has " << indices.size() << " triangles" << "\n";
	std::cout << "\t'" << filePath << "' has " << vertices.size() << " vertices" << "\n\n\n\n\n";
}

void Mesh::GenBoundingBox()
{
	struct Node root;

	for (int i = 0; i < vertices.size(); ++i)
	{
		glm::vec3 vertex = glm::vec3(vertices[i].x, vertices[i].y, vertices[i].z);

		if (vertex.x > root.boundsMax[0]) root.boundsMax[0] = vertex.x;
		if (vertex.y > root.boundsMax[1]) root.boundsMax[1] = vertex.y;
		if (vertex.z > root.boundsMax[2]) root.boundsMax[2] = vertex.z;

		if (vertex.x < root.boundsMin[0]) root.boundsMin[0] = vertex.x;
		if (vertex.y < root.boundsMin[1]) root.boundsMin[1] = vertex.y;
		if (vertex.z < root.boundsMin[2]) root.boundsMin[2] = vertex.z;
	}

	nodes.push_back(root);

	std::cout << "\n\tBounds min: " << "x: " << root.boundsMin[0] << ", y: " << root.boundsMin[1] << ", z: " << root.boundsMin[2] << "\n";
	std::cout << "\tBounds max: " << "x: " << root.boundsMax[0] << ", y: " << root.boundsMax[1] << ", z: " << root.boundsMax[2] << "\n\n\n\n\n";
}