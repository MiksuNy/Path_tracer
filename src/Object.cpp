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
	p1.x = 0.0f; p1.y = 0.0f; p1.z = 0.0f;
	p2.x = 0.0f; p2.y = 0.0f; p2.z = 0.0f;
	p3.x = 0.0f; p3.y = 0.0f; p3.z = 0.0f;
}

Triangle::Triangle(struct Scene& scene, glm::vec3 _p1, glm::vec3 _p2, glm::vec3 _p3, unsigned int materialIndex)
{
    p1.x = _p1.x; p1.y = _p1.y; p1.z = _p1.z;
	p2.x = _p2.x; p2.y = _p2.y; p2.z = _p2.z;
	p3.x = _p3.x; p3.y = _p3.y; p3.z = _p3.z;
	this->materialIndex = materialIndex;

	scene.triangles.push_back(*this);
}

glm::vec3 Triangle::Center()
{
	return glm::vec3(p1 + p2 + p3) * 0.3333f;
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

Mesh::Mesh(const char* filePath, uint32_t materialIndex)
{
	this->materialIndex = materialIndex;
	Load(filePath);
	GenBoundingBox();
	SplitNode(0, 1);

	std::cout << "\n\nMesh BVH size: " << nodes.size() << "\n\n";

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
			glm::uvec4 index;
			sscanf_s(line.c_str(), "f %i %i %i", &index.x, &index.y, &index.z);

			// Do this because obj files start indices at 1
			index.x -= 1;
			index.y -= 1;
			index.z -= 1;
			index.w = 0;

			indices.push_back(index);
		}
	}

	inFile.close();
	double timeAfterLoad = glfwGetTime();

	// Make the mesh triangle buffer for ray tracing shader
	for (int i = 0; i < indices.size(); ++i)
	{
		Triangle tri;
		tri.p1 = vertices[indices[i].x];
		tri.p2 = vertices[indices[i].y];
		tri.p3 = vertices[indices[i].z];
		tri.materialIndex = this->materialIndex;
		tris.push_back(tri);
	}

	std::cout << "\n\n\n\t'" << filePath << "' took " << timeAfterLoad - timeBeforeLoad << " seconds to load" << "\n";
	std::cout << "\t'" << filePath << "' has " << indices.size() << " triangles" << "\n";
	std::cout << "\t'" << filePath << "' has " << vertices.size() << " vertices" << "\n\n\n\n\n";
	
	vertices.~vector();
	indices.~vector();
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
	root.triIndex = 0;
	nodes.push_back(root);

	std::cout << "\n\tBounds min: " << "x: " << root.boundsMin.x << ", y: " << root.boundsMin.y << ", z: " << root.boundsMin.z << "\n";
	std::cout << "\tBounds max: " << "x: " << root.boundsMax.x << ", y: " << root.boundsMax.y << ", z: " << root.boundsMax.z << "\n\n\n\n\n";
}

void Mesh::SplitNode(unsigned int nodeIndex, unsigned int depth)
{
	Node& parent = nodes[nodeIndex];

	if (parent.numTris <= 2u || depth <= 0) return;

	Node childA;
	Node childB;

	float sizeX = abs(parent.boundsMin.x) + abs(parent.boundsMax.x);
	float sizeY = abs(parent.boundsMin.y) + abs(parent.boundsMax.y);
	float sizeZ = abs(parent.boundsMin.z) + abs(parent.boundsMax.z);

	int splitAxis = (sizeX > sizeY && sizeX > sizeZ) ? 0 : (sizeY > sizeX && sizeY > sizeZ) ? 1 : (sizeZ > sizeX && sizeZ > sizeY) ? 2 : 0;

	unsigned int i = parent.triIndex;
	unsigned int j = i + parent.numTris - 1u;
	while (i <= j)
	{
		bool inA = tris[i].Center()[splitAxis] <= (parent.boundsMin[splitAxis] + parent.boundsMax[splitAxis]) / 2;
		if (inA) i++;
		else std::swap(tris[i], tris[j--]);
	}

	unsigned int triCountA = i - parent.triIndex;
	unsigned int triCountB = parent.numTris - triCountA;
	if (triCountA == 0u || triCountA == parent.numTris) return;

	childA.triIndex = parent.triIndex;
	childA.numTris = triCountA;
	childB.triIndex = i;
	childB.numTris = triCountB;

	unsigned int triIndex = childA.triIndex;
	while (triIndex < childA.numTris - 1u)
	{
		childA.GrowBounds(tris[triIndex]);
		triIndex++;
	}
	for (unsigned int i = triIndex; i < parent.numTris - 1u; i++)
	{
		childB.GrowBounds(tris[i]);
	}

	nodes.push_back(childA);
	nodes.push_back(childB);

	parent.numTris = 0u;
	parent.childrenIndex = nodes.size() - 2u;

	std::cout << "Children start at index: " << parent.childrenIndex << std::endl;
	std::cout << "Parent has " << parent.numTris << " triangles" << std::endl;
	std::cout << "Child A has " << childA.numTris << " triangles" << std::endl;
	std::cout << "Child B has " << childB.numTris << " triangles" << std::endl;
	std::cout << "Child A max bounds: " << childA.boundsMax.x << ", " << childA.boundsMax.y << ", " << childA.boundsMax.z << std::endl;
	std::cout << "Child A min bounds: " << childA.boundsMin.x << ", " << childA.boundsMin.y << ", " << childA.boundsMin.z << std::endl;
	std::cout << "Child B max bounds: " << childB.boundsMax.x << ", " << childB.boundsMax.y << ", " << childB.boundsMax.z << std::endl;
	std::cout << "Child B min bounds: " << childB.boundsMin.x << ", " << childB.boundsMin.y << ", " << childB.boundsMin.z << std::endl << std::endl;

	SplitNode(usedNodes++, depth - 1);
	SplitNode(usedNodes++, depth - 1);
}