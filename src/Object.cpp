#include "Object.h"

void Scene::SetupSSBOs()
{
	glGenBuffers(1, &materialSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, materials.size() * sizeof(Material), materials.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, materialSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Scene::UpdateSSBOs()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, materials.size() * sizeof(Material), materials.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, materialSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

glm::vec3 Triangle::Center()
{
	return glm::vec3(p1 + p2 + p3) * 0.33333f;
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

Mesh::Mesh(const char* filePath, std::vector<Material>& materials)
{
	Load(filePath, materials);
	GenBoundingBox();
	SplitNode(0, 6);

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

void Mesh::Load(const char* filePath, std::vector<Material>& materials)
{
	double timeBeforeLoad = glfwGetTime();

	std::string tempFilePath = filePath;

	std::ifstream objFile(tempFilePath + ".obj");
	if (!objFile.is_open())
	{
		std::cerr << "File not found: " << filePath << std::endl;
		return;
	}
	std::string line;

	LoadMtl((tempFilePath + ".mtl").c_str(), materials);
	if (materials.empty())
	{
		std::cerr << "No materials found for " << (tempFilePath + ".mtl").c_str() << std::endl;
		return;
	}

	char currMaterialName[64];

	while (getline(objFile, line))
	{
		if (line.substr(0, 2) == "v ") // vertices
		{
			glm::vec4 vertex;
			sscanf_s(line.c_str(), "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);

			vertices.push_back(vertex);
		}
		else if (line.substr(0, 2) == "f ") // indices
		{
			glm::uvec4 index;
			sscanf_s(line.c_str(), "f %i %i %i", &index.x, &index.y, &index.z);

			// Do this because obj files start indices at 1
			index.x -= 1u;
			index.y -= 1u;
			index.z -= 1u;

			Triangle tri;
			tri.p1 = vertices[index.x];
			tri.p2 = vertices[index.y];
			tri.p3 = vertices[index.z];
			unsigned int materialIndex = 0;
			while (materialIndex < materialNames.size() - 1)
			{
				materialIndex++;
				if (strcmp(currMaterialName, materialNames[materialIndex].c_str()) == 0)
				{
					tri.materialIndex = materialIndex;
					break;
				}
			}
			tris.push_back(tri);
		}
		else if (line.substr(0, 7) == "usemtl ")
		{
			sscanf_s(line.c_str(), "usemtl %s", &currMaterialName, 64);
		}

	}

	objFile.close();

	double timeAfterLoad = glfwGetTime();

	std::cout << "\n\n\n\t'" << filePath << "' took " << timeAfterLoad - timeBeforeLoad << " seconds to load" << "\n";
	std::cout << "\t'" << filePath << "' has " << vertices.size() << " vertices" << "\n";
	std::cout << "\t'" << filePath << "' has " << tris.size() << " triangles" << "\n\n\n\n";
	
	vertices.~vector();
}

void Mesh::LoadMtl(const char* filePath, std::vector<Material>& materials)
{
	std::ifstream mtlFile(filePath);
	std::string line;

	if (!mtlFile.is_open())
	{
		std::cerr << "File not found: " << filePath << std::endl;
	}
	while (getline(mtlFile, line))
	{
		if (line.substr(0, 7) == "newmtl ")
		{
			materials.push_back(Material{});
			Material& newMat = materials[materials.size() - 1];

			char matName[64];
			sscanf_s(line.c_str(), "newmtl %s", &matName, 64);
			materialNames.push_back(matName);

			while (getline(mtlFile, line))
			{
				// Materials always have a blank line between them so we break here to read the name of the next material
				if (line.substr(0, 1) == "") break;

				else if (line.substr(0, 3) == "Kd ") // Base color
				{
					sscanf_s(line.c_str(), "Kd %f %f %f", &newMat.baseColor.x, &newMat.baseColor.y, &newMat.baseColor.z);
				}
				else if (line.substr(0, 3) == "Ke ") // Emission color & strength
				{
					glm::vec4 emissionColor;
					sscanf_s(line.c_str(), "Ke %f %f %f", &emissionColor.x, &emissionColor.y, &emissionColor.z);
					newMat.emissionStrength = std::max(emissionColor.x, std::max(emissionColor.y, emissionColor.z));

					if (newMat.emissionStrength > 0) newMat.emissionColor = emissionColor / newMat.emissionStrength;
				}
				else if (line.substr(0, 3) == "Ni ") // Index of refraction
				{
					sscanf_s(line.c_str(), "Ni %f", &newMat.ior);
				}
				else if (line.substr(0, 3) == "Pr ") // Roughness (smoothness)
				{
					float roughness;
					sscanf_s(line.c_str(), "Pr %f", &roughness);
					newMat.roughness = roughness;
				}
				else if (line.substr(0, 3) == "Pm ") // Metallic
				{
					float metallic;
					sscanf_s(line.c_str(), "Pm %f", &metallic);
					newMat.metallic = metallic;
				}
				else if (line.substr(0, 3) == "Tf ") // Refraction amount
				{
					float refraction[3];
					sscanf_s(line.c_str(), "Tf %f %f %f", &refraction[0], &refraction[1], &refraction[2]);
					newMat.refractionAmount = std::max(refraction[0], std::max(refraction[1], refraction[2]));
				}
			}
		}
	}

	mtlFile.close();
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