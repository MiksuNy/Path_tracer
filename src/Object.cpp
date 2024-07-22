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
    p1.x = _p1.x; p1.y = _p1.y; p1.z = _p1.z; p1.w = 0.0f;
	p2.x = _p2.x; p2.y = _p2.y; p2.z = _p2.z; p2.w = 0.0f;
	p3.x = _p3.x; p3.y = _p3.y; p3.z = _p3.z; p3.w = 0.0f;
	
	program.Use();
	program.SetUniform4f(std::string(name).append(".p1").c_str(),							this->p1);
	program.SetUniform4f(std::string(name).append(".p2").c_str(),							this->p2);
	program.SetUniform4f(std::string(name).append(".p3").c_str(),							this->p3);
	program.SetUniform3f(std::string(name).append(".material.baseColor").c_str(),			material.baseColor);
	program.SetUniform1f(std::string(name).append(".material.roughness").c_str(),			material.roughness);
	program.SetUniform3f(std::string(name).append(".material.emissionColor").c_str(),		material.emissionColor);
	program.SetUniform1f(std::string(name).append(".material.emissionStrength").c_str(),	material.emissionStrength);
	program.SetUniform1f(std::string(name).append(".material.ior").c_str(),					material.ior);
	program.SetUniform1f(std::string(name).append(".material.refractionAmount").c_str(),	material.refractionAmount);
	program.Unuse();
}

glm::vec3 Triangle::Center()
{
	float cX = (p1.x + p2.x + p3.x) / 3;
	float cY = (p1.y + p2.y + p3.y) / 3;
	float cZ = (p1.z + p2.z + p3.z) / 3;
	return glm::vec3(cX, cY, cZ);
}

Mesh::Mesh(const char* filePath, Material material)
{
	this->material = material;
	this->Load(filePath);
	this->GenBoundingBox();

	this->SplitNode(nodes[0]);
	this->SplitNode(nodes[1]);
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

	//tempTri.p1 = vertices[indices[i].x].xyz;
	//tempTri.p2 = vertices[indices[i].y].xyz;
	//tempTri.p3 = vertices[indices[i].z].xyz;

	for (int i = 0; i < indices.size(); ++i)
	{
		Triangle tempTri;
		tempTri.p1 = vertices[indices[i].x];
		tempTri.p2 = vertices[indices[i].y];
		tempTri.p3 = vertices[indices[i].z];
		tempTri.material = this->material;
		tris.push_back(tempTri);
	}

	double timeAfterLoad = glfwGetTime();
	std::cout << "\n\n\n\t'" << filePath << "' took " << timeAfterLoad - timeBeforeLoad << " seconds to load" << "\n";
	std::cout << "\t'" << filePath << "' has " << indices.size() << " triangles" << "\n";
	std::cout << "\t'" << filePath << "' has " << vertices.size() << " vertices" << "\n\n\n\n\n";
}

void Mesh::GenBoundingBox()
{
	struct Node self;

	for (int i = 0; i < vertices.size(); ++i)
	{
		glm::vec3 vertex = glm::vec3(vertices[i].x, vertices[i].y, vertices[i].z);

		if (vertex.x > self.boundsMax[0]) self.boundsMax[0] = vertex.x;
		if (vertex.y > self.boundsMax[1]) self.boundsMax[1] = vertex.y;
		if (vertex.z > self.boundsMax[2]) self.boundsMax[2] = vertex.z;

		if (vertex.x < self.boundsMin[0]) self.boundsMin[0] = vertex.x;
		if (vertex.y < self.boundsMin[1]) self.boundsMin[1] = vertex.y;
		if (vertex.z < self.boundsMin[2]) self.boundsMin[2] = vertex.z;
	}

	nodes.push_back(self);

	std::cout << "\n\tBounds min: " << "x: " << self.boundsMin[0] << ", y: " << self.boundsMin[1] << ", z: " << self.boundsMin[2] << "\n";
	std::cout << "\tBounds max: " << "x: " << self.boundsMax[0] << ", y: " << self.boundsMax[1] << ", z: " << self.boundsMax[2] << "\n\n\n\n\n";
}

void Mesh::SplitNode(Node parent)
{
	struct Node childA;
	struct Node childB;

	float sizeX = abs(parent.boundsMin[0]) + abs(parent.boundsMax[0]);
	float sizeY = abs(parent.boundsMin[1]) + abs(parent.boundsMax[1]);
	float sizeZ = abs(parent.boundsMin[2]) + abs(parent.boundsMax[2]);

	std::cout << "Size X: " << sizeX << "\n";
	std::cout << "Size Y: " << sizeY << "\n";
	std::cout << "Size Z: " << sizeZ << "\n";

	int splitAxis = (sizeX > sizeY && sizeX > sizeZ) ? 0 : (sizeY > sizeX && sizeY > sizeZ) ? 1 : (sizeZ > sizeX && sizeZ > sizeY) ? 2 : 0;

	const char* axis = "";
	(splitAxis == 0) ? axis = "X" : (splitAxis == 1) ? axis = "Y" : (splitAxis == 2) ? axis = "Z" : axis = "WTF?";
	std::cout << "Split axis: " << axis << "\n";

	for (int i = 0; i < vertices.size(); ++i)
	{
		glm::vec3 vertex = glm::vec3(vertices[i].x, vertices[i].y, vertices[i].z);

		if (vertex[splitAxis] >= (parent.boundsMin[splitAxis] + parent.boundsMax[splitAxis]) / 2)
		{
			if (vertex.x > childA.boundsMax[0]) childA.boundsMax[0] = vertex.x;
			if (vertex.y > childA.boundsMax[1]) childA.boundsMax[1] = vertex.y;
			if (vertex.z > childA.boundsMax[2]) childA.boundsMax[2] = vertex.z;

			if (vertex.x < childA.boundsMin[0]) childA.boundsMin[0] = vertex.x;
			if (vertex.y < childA.boundsMin[1]) childA.boundsMin[1] = vertex.y;
			if (vertex.z < childA.boundsMin[2]) childA.boundsMin[2] = vertex.z;
		}
		if (vertex[splitAxis] <= (parent.boundsMin[splitAxis] + parent.boundsMax[splitAxis]) / 2)
		{
			if (vertex.x > childB.boundsMax[0]) childB.boundsMax[0] = vertex.x;
			if (vertex.y > childB.boundsMax[1]) childB.boundsMax[1] = vertex.y;
			if (vertex.z > childB.boundsMax[2]) childB.boundsMax[2] = vertex.z;

			if (vertex.x < childB.boundsMin[0]) childB.boundsMin[0] = vertex.x;
			if (vertex.y < childB.boundsMin[1]) childB.boundsMin[1] = vertex.y;
			if (vertex.z < childB.boundsMin[2]) childB.boundsMin[2] = vertex.z;
		}
	}

	nodes.push_back(childA);
	nodes.push_back(childB);

	std::cout << "\n\tChild A Bounds min: " << "x: " << childA.boundsMin[0] << ", y: " << childA.boundsMin[1] << ", z: " << childA.boundsMin[2] << "\n";
	std::cout << "\tChild A Bounds max: " << "x: " << childA.boundsMax[0] << ", y: " << childA.boundsMax[1] << ", z: " << childA.boundsMax[2] << "\n";

	std::cout << "\n\tChild B Bounds min: " << "x: " << childB.boundsMin[0] << ", y: " << childB.boundsMin[1] << ", z: " << childB.boundsMin[2] << "\n";
	std::cout << "\tChild B Bounds max: " << "x: " << childB.boundsMax[0] << ", y: " << childB.boundsMax[1] << ", z: " << childB.boundsMax[2] << "\n";
}