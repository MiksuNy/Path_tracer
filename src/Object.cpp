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

Triangle::Triangle()
{
	p1.x = 0.0f; p1.y = 0.0f; p1.z = 0.0f; p1.w = 0.0f;
	p2.x = 0.0f; p2.y = 0.0f; p2.z = 0.0f; p1.w = 0.0f;
	p3.x = 0.0f; p3.y = 0.0f; p3.z = 0.0f; p1.w = 0.0f;
}

Triangle::Triangle(struct Program& program, const char* name, glm::vec3 _p1, glm::vec3 _p2, glm::vec3 _p3, struct Material& material)
{
    p1.x = _p1.x; p1.y = _p1.y; p1.z = _p1.z;
	p2.x = _p2.x; p2.y = _p2.y; p2.z = _p2.z;
	p3.x = _p3.x; p3.y = _p3.y; p3.z = _p3.z;

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
	this->GenBVH();
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
	for (int i = 0; i < this->vertices.size(); i+=4)
	{
		if (this->vertices[i] > boundsMax[0]) boundsMax[0] = this->vertices[i];
		else if (this->vertices[i + 1] > boundsMax[1]) boundsMax[1] = this->vertices[i + 1];
		else if (this->vertices[i + 2] > boundsMax[2]) boundsMax[2] = this->vertices[i + 2];

		else if (this->vertices[i] < boundsMin[0]) boundsMin[0] = this->vertices[i];
		else if (this->vertices[i + 1] < boundsMin[1]) boundsMin[1] = this->vertices[i + 1];
		else if (this->vertices[i + 2] < boundsMin[2]) boundsMin[2] = this->vertices[i + 2];
	}

	std::cout << "\n\tBounds min: " << "x: " << boundsMin[0] << ", y: " << boundsMin[1] << ", z: " << boundsMin[2] << "\n";
	std::cout << "\tBounds max: " << "x: " << boundsMax[0] << ", y: " << boundsMax[1] << ", z: " << boundsMax[2] << "\n\n\n\n\n";
}

void Mesh::GenBVH()
{
	int numVertsA = 0;
	int numVertsB = 0;

	for (int i = 0; i < this->vertices.size(); i += 4)
	{
		if (this->vertices[i] < 0.0)
		{
			if (this->vertices[i] > childA.boundsMax[0]) childA.boundsMax[0] = this->vertices[i];
			else if (this->vertices[i + 1] > childA.boundsMax[1]) childA.boundsMax[1] = this->vertices[i + 1];
			else if (this->vertices[i + 2] > childA.boundsMax[2]) childA.boundsMax[2] = this->vertices[i + 2];

			else if (this->vertices[i] < childA.boundsMin[0]) childA.boundsMin[0] = this->vertices[i];
			else if (this->vertices[i + 1] < childA.boundsMin[1]) childA.boundsMin[1] = this->vertices[i + 1];
			else if (this->vertices[i + 2] < childA.boundsMin[2]) childA.boundsMin[2] = this->vertices[i + 2];

			numVertsA++;
		}
		else if (this->vertices[i] > 0.0)
		{
			if (this->vertices[i] > childB.boundsMax[0]) childB.boundsMax[0] = this->vertices[i];
			else if (this->vertices[i + 1] > childB.boundsMax[1]) childB.boundsMax[1] = this->vertices[i + 1];
			else if (this->vertices[i + 2] > childB.boundsMax[2]) childB.boundsMax[2] = this->vertices[i + 2];

			else if (this->vertices[i] < childB.boundsMin[0]) childB.boundsMin[0] = this->vertices[i];
			else if (this->vertices[i + 1] < childB.boundsMin[1]) childB.boundsMin[1] = this->vertices[i + 1];
			else if (this->vertices[i + 2] < childB.boundsMin[2]) childB.boundsMin[2] = this->vertices[i + 2];

			numVertsB++;
		}
	}

	std::cout << "\n\tChild B Bounds min: " << "x: " << childB.boundsMin[0] << ", y: " << childB.boundsMin[1] << ", z: " << childB.boundsMin[2] << "\n";
	std::cout << "\tChild B Bounds max: " << "x: " << childB.boundsMax[0] << ", y: " << childB.boundsMax[1] << ", z: " << childB.boundsMax[2] << "\n\n\n\n\n";

	std::cout << "\n\tChild A Bounds min: " << "x: " << childA.boundsMin[0] << ", y: " << childA.boundsMin[1] << ", z: " << childA.boundsMin[2] << "\n";
	std::cout << "\tChild A Bounds max: " << "x: " << childA.boundsMax[0] << ", y: " << childA.boundsMax[1] << ", z: " << childA.boundsMax[2] << "\n\n\n\n\n";

	std::cout << "\n\tChild A vertex count: " << numVertsA << ", Child B vertex count: " << numVertsB << "\n\n\n\n\n";
}