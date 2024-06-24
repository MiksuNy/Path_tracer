#include "Object.h"

Sphere::Sphere(struct Program& program, const char* name, glm::vec3 pos, float rad, struct Material& material)
{
    position = pos;
    radius = rad;

	program.Use();
	program.SetUniform3f(std::string(name).append(".position").c_str(),						this->position);
	program.SetUniform1f(std::string(name).append(".radius").c_str(),						this->radius);
	program.SetUniform3f(std::string(name).append(".material.baseColor").c_str(),			material.baseColor);
	program.SetUniform1f(std::string(name).append(".material.roughness").c_str(),			material.roughness);
	program.SetUniform3f(std::string(name).append(".material.emissionColor").c_str(),		material.emissionColor);
	program.SetUniform1f(std::string(name).append(".material.emissionStrength").c_str(),	material.emissionStrength);
	program.SetUniform1f(std::string(name).append(".material.ior").c_str(),					material.ior);
	program.SetUniform1f(std::string(name).append(".material.refractionAmount").c_str(),	material.refractionAmount);
	program.Unuse();
}

Triangle::Triangle(struct Program& program, const char* name, glm::vec3 _p1, glm::vec3 _p2, glm::vec3 _p3, struct Material& material)
{
    p1 = _p1;
    p2 = _p2;
    p3 = _p3;

	program.Use();
	program.SetUniform3f(std::string(name).append(".p1").c_str(),							this->p1);
	program.SetUniform3f(std::string(name).append(".p2").c_str(),							this->p2);
	program.SetUniform3f(std::string(name).append(".p3").c_str(),							this->p3);
	program.SetUniform3f(std::string(name).append(".material.baseColor").c_str(),			material.baseColor);
	program.SetUniform1f(std::string(name).append(".material.roughness").c_str(),			material.roughness);
	program.SetUniform3f(std::string(name).append(".material.emissionColor").c_str(),		material.emissionColor);
	program.SetUniform1f(std::string(name).append(".material.emissionStrength").c_str(),	material.emissionStrength);
	program.SetUniform1f(std::string(name).append(".material.ior").c_str(),					material.ior);
	program.SetUniform1f(std::string(name).append(".material.refractionAmount").c_str(),	material.refractionAmount);
	program.Unuse();
}

Mesh::Mesh(const char* filePath)
{
	this->Load(filePath);
	this->GenBoundingBox();
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
	else
	{
		while (getline(inFile, line))
		{
			if (line.substr(0, 2) == "v ")
			{
				float x, y, z, w;
				sscanf_s(line.c_str(), "v %f %f %f", &x, &y, &z); // vertices
				w = 1.0f;

				vertices.push_back(x);
				vertices.push_back(y);
				vertices.push_back(z);
				vertices.push_back(w);
			}
			else if (line.substr(0, 2) == "f ") // indices
			{
				int i1, i2, i3, i4;
				sscanf_s(line.c_str(), "f %i %i %i", &i1, &i2, &i3);
				i4 = 0;

				indices.push_back(i1 - 1);
				indices.push_back(i2 - 1);
				indices.push_back(i3 - 1);
				indices.push_back(i4);
			}
		}

		inFile.close();

		double timeAfterLoad = glfwGetTime();
		std::cout << "\n\n\n\t'" << filePath << "' took " << timeAfterLoad - timeBeforeLoad << " seconds to load" << "\n";
		std::cout << "\t'" << filePath << "' has " << indices.size() / 4 << " triangles" << "\n\n\n\n\n";
	}
}

void Mesh::GenBoundingBox()
{
	float maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;
	float minX = 0.0f, minY = 0.0f, minZ = 0.0f;
	for (int i = 0; i < this->vertices.size() - 4; i+=4)
	{
		if (this->vertices[i] > maxX) maxX = this->vertices[i];
		if (this->vertices[i + 1] > maxY) maxY = this->vertices[i + 1];
		if (this->vertices[i + 2] > maxZ) maxZ = this->vertices[i + 2];
	}
	for (int i = 0; i < this->vertices.size() - 4; i += 4)
	{
		if (this->vertices[i] < minX) minX = this->vertices[i];
		if (this->vertices[i + 1] < minY) minY = this->vertices[i + 1];
		if (this->vertices[i + 2] < minZ) minZ = this->vertices[i + 2];
	}
	this->boundsMax[0] = maxX;
	this->boundsMax[1] = maxY;
	this->boundsMax[2] = maxZ;
	this->boundsMax[3] = 1.0f;

	this->boundsMin[0] = minX;
	this->boundsMin[1] = minY;
	this->boundsMin[2] = minZ;
	this->boundsMin[3] = 1.0f;

	std::cout << "\n\tBounds min: " << "x: " << boundsMin[0] << ", y: " << boundsMin[1] << ", z: " << boundsMin[2] << "\n";
	std::cout << "\tBounds max: " << "x: " << boundsMax[0] << ", y: " << boundsMax[1] << ", z: " << boundsMax[2] << "\n\n\n\n\n";
}