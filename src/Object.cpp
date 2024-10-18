#include "Object.h"

void Scene::SetupSSBOs()
{
	glGenBuffers(1, &materialSSBO);
	glGenBuffers(1, &sphereSSBO);
	glGenBuffers(1, &triangleSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, materials.size() * sizeof(Material), materials.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, materialSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, spheres.size() * sizeof(Sphere), spheres.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, sphereSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(Triangle), triangles.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, triangleSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Scene::UpdateSSBOs()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, materials.size() * sizeof(Material), materials.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, materialSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, spheres.size() * sizeof(Sphere), spheres.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, sphereSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(Triangle), triangles.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, triangleSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

Sphere::Sphere(struct Scene& scene, glm::vec3 pos, float rad, unsigned int materialIndex)
{
    this->position = pos;
    this->radius = rad;
	this->materialIndex = materialIndex;

	scene.spheres.push_back(*this);
}

Triangle::Triangle()
{
	p1.x = 0.0f; p1.y = 0.0f; p1.z = 0.0f; p1.w = 0.0f;
	p2.x = 0.0f; p2.y = 0.0f; p2.z = 0.0f; p1.w = 0.0f;
	p3.x = 0.0f; p3.y = 0.0f; p3.z = 0.0f; p1.w = 0.0f;
}

Triangle::Triangle(struct Scene& scene, glm::vec3 _p1, glm::vec3 _p2, glm::vec3 _p3, unsigned int materialIndex)
{
    p1.x = _p1.x; p1.y = _p1.y; p1.z = _p1.z; p1.w = 0.0f;
	p2.x = _p2.x; p2.y = _p2.y; p2.z = _p2.z; p2.w = 0.0f;
	p3.x = _p3.x; p3.y = _p3.y; p3.z = _p3.z; p3.w = 0.0f;
	this->materialIndex = materialIndex;

	scene.triangles.push_back(*this);
}

glm::vec3 Triangle::Center()
{
	float cX = (p1.x + p2.x + p3.x) / 3;
	float cY = (p1.y + p2.y + p3.y) / 3;
	float cZ = (p1.z + p2.z + p3.z) / 3;
	return glm::vec3(cX, cY, cZ);
}

Mesh::Mesh(const char* filePath, uint32_t materialIndex)
{
	this->materialIndex = materialIndex;
	Load(filePath);
	GenBoundingBox();
	SplitNode(nodes[0], 1);

	std::cout << "\n\nMesh BVH size: " << nodes.size() << "\n\n";

	GLuint meshSSBO, bvhSSBO;

	glGenBuffers(1, &meshSSBO);
	glGenBuffers(1, &bvhSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tris.size() * sizeof(Triangle), tris.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, nodes.size() * sizeof(Node), nodes.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bvhSSBO);
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
	double timeAfterLoad = glfwGetTime();

	// Make the triangle buffer for raytracing shader
	for (int i = 0; i < indices.size(); ++i)
	{
		Triangle tempTri;
		tempTri.p1 = vertices[indices[i].x];
		tempTri.p2 = vertices[indices[i].y];
		tempTri.p3 = vertices[indices[i].z];
		tempTri.materialIndex = this->materialIndex;
		tris.push_back(tempTri);
	}

	std::cout << "\n\n\n\t'" << filePath << "' took " << timeAfterLoad - timeBeforeLoad << " seconds to load" << "\n";
	std::cout << "\t'" << filePath << "' has " << indices.size() << " triangles" << "\n";
	std::cout << "\t'" << filePath << "' has " << vertices.size() << " vertices" << "\n\n\n\n\n";
}

void Mesh::GenBoundingBox()
{
	Node root;

	for (int i = 0; i < tris.size(); ++i)
	{
		root.GrowBounds(tris[i]);
	}
	root.numTris = tris.size();
	root.childrenIndex = 1;
	nodes.push_back(root);

	std::cout << "\n\tBounds min: " << "x: " << root.boundsMin.x << ", y: " << root.boundsMin.y << ", z: " << root.boundsMin.z << "\n";
	std::cout << "\tBounds max: " << "x: " << root.boundsMax.x << ", y: " << root.boundsMax.y << ", z: " << root.boundsMax.z << "\n\n\n\n\n";
}

void Node::GrowBounds(Triangle tri)
{
	boundsMin = glm::min(boundsMin, tri.p1);
	boundsMax = glm::max(boundsMax, tri.p1);

	boundsMin = glm::min(boundsMin, tri.p2);
	boundsMax = glm::max(boundsMax, tri.p2);

	boundsMin = glm::min(boundsMin, tri.p3);
	boundsMax = glm::max(boundsMax, tri.p3);
}

void Mesh::SplitNode(Node parent, int depth)
{
	if (depth <= 0) return;
	
	parent.childrenIndex = nodes.size();

	Node childA;
	childA.triIndex = parent.triIndex;
	Node childB;
	childB.triIndex = parent.triIndex;

	float sizeX = abs(parent.boundsMin.x) + abs(parent.boundsMax.x);
	float sizeY = abs(parent.boundsMin.y) + abs(parent.boundsMax.y);
	float sizeZ = abs(parent.boundsMin.z) + abs(parent.boundsMax.z);

	//std::cout << "\n\tMax X: " << sizeX << "\n";
	//std::cout << "\n\tMax Y: " << sizeY << "\n";
	//std::cout << "\n\tMax Z: " << sizeZ << "\n\n\n";

	int splitAxis = (sizeX > glm::max(sizeY, sizeZ)) ? 0 : (sizeY > glm::max(sizeX, sizeZ)) ? 1 : 2;
	//std::cout << "Split axis: " << ((splitAxis == 0) ? "X" : (splitAxis == 1) ? "Y" : (splitAxis == 2) ? "Z" : "This isn't supposed to happen") << std::endl;

	for (int i = parent.triIndex; i < parent.triIndex + parent.numTris; i++)
	{
		bool inA = tris[i].Center()[splitAxis] < (parent.boundsMin[splitAxis] + parent.boundsMax[splitAxis]) / 2;
		childA.GrowBounds(tris[i]);
		childA.numTris++;

		if (inA)
		{
			int swap = childA.triIndex + childA.numTris - 1;
			std::swap(tris[i], tris[swap]);
			childB.triIndex++;
		}
	}

	nodes[parent.childrenIndex - 1].childrenIndex = parent.childrenIndex;
	nodes.push_back(childA);
	nodes.push_back(childB);

	std::cout << "\n\tChildren start at index: " << nodes[parent.childrenIndex - 1].childrenIndex << "\n";

	std::cout << "\n\tChild A bounds min: " << "x: " << childA.boundsMin.x << ", y: " << childA.boundsMin.y << ", z: " << childA.boundsMin.z << "\n";
	std::cout << "\tChild A bounds max: " << "x: " << childA.boundsMax.x << ", y: " << childA.boundsMax.y << ", z: " << childA.boundsMax.z << "\n\n\n\n\n";

	std::cout << "\n\tChild B bounds min: " << "x: " << childB.boundsMin.x << ", y: " << childB.boundsMin.y << ", z: " << childB.boundsMin.z << "\n";
	std::cout << "\tChild B bounds max: " << "x: " << childB.boundsMax.x << ", y: " << childB.boundsMax.y << ", z: " << childB.boundsMax.z << "\n\n\n\n\n";

	std::cout << "\n\tChild A tri start index: " << nodes[parent.childrenIndex].triIndex << "\n";
	std::cout << "\n\tChild B tri start index: " << nodes[parent.childrenIndex + 1].triIndex << "\n";

	std::cout << "\n\tParent tris: " << nodes[parent.childrenIndex - 1].numTris << "\n";
	std::cout << "\n\tChild A tris: " << nodes[parent.childrenIndex].numTris << "\n";
	std::cout << "\n\tChild B tris: " << nodes[parent.childrenIndex + 1].numTris << "\n\n\n\n\n";

	if (nodes[parent.childrenIndex + 1].numTris > 1) SplitNode(nodes[parent.childrenIndex + 1], depth - 1);
	if (nodes[parent.childrenIndex].numTris > 1) SplitNode(nodes[parent.childrenIndex], depth - 1);
}