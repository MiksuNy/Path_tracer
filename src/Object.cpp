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
	program.SetUniform1i(std::string(name).append(".material.isRefractive").c_str(),		material.isRefractive);
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
	program.SetUniform1i(std::string(name).append(".material.isRefractive").c_str(),		material.isRefractive);
	program.SetUniform1f(std::string(name).append(".material.ior").c_str(),					material.ior);
	program.SetUniform1f(std::string(name).append(".material.refractionAmount").c_str(),	material.refractionAmount);
	program.Unuse();
}

Mesh::Mesh(const char* filePath)
{
	this->Load(filePath);
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
				glm::highp_vec4 vertex;
				sscanf_s(line.c_str(), "v %f %f %f", &vertex.x, &vertex.y, &vertex.z); // vertices
				vertex.w = 1.0f;

				vertices.push_back(vertex.x);
				vertices.push_back(vertex.y);
				vertices.push_back(vertex.z);
				vertices.push_back(vertex.w);
			}
			else if (line.substr(0, 2) == "f ") // indices
			{
				int i1, i2, i3;
				sscanf_s(line.c_str(), "f %i %i %i", &i1, &i2, &i3);

				indices.push_back(i1 - 1);
				indices.push_back(i2 - 1);
				indices.push_back(i3 - 1);
			}
		}

		inFile.close();

		double timeAfterLoad = glfwGetTime();
		std::cout << "\n\n\n\t'" << filePath << "' took " << timeAfterLoad - timeBeforeLoad << " seconds to load" << "\n";
		std::cout << "\t'" << filePath << "' has " << vertices.size() / 4 << " vertices" << "\n\n\n\n\n";
	}
}