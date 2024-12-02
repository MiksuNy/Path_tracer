#pragma once

#include <vector>
#include <glm.hpp>
#include <map>

#include "Shader.h"

// TODO: SSBOs need to be padded to vec4 leaving an unused fourth component, is there a way to avoid this?

// Needs to be padded to multiple of 16 bytes for SSBO usage
struct alignas(16) Material
{
    glm::vec4 baseColor = glm::vec4(1);
    glm::vec4 emissionColor = glm::vec4(1);
    float roughness = 0.0f;
    float metallic = 0.0f;
    float emissionStrength = 0.0f;
    float ior = 1.5f;
    float refractionAmount = 0.0f;

private:
    int pad[3];
};

// Needs to be padded to multiple of 16 bytes for SSBO usage
struct alignas(16) Triangle
{
    glm::vec4 p1 = glm::vec4(0);
    glm::vec4 p2 = glm::vec4(0);
    glm::vec4 p3 = glm::vec4(0);
    unsigned int materialIndex = 0;

    glm::vec3 Center();

private:
    int pad[3];
};

struct Scene
{
    std::vector<Material> materials;

    void SetupSSBOs();
    void UpdateSSBOs();

private:
    GLuint materialSSBO;
};

// Needs to be padded to multiple of 16 bytes for SSBO usage
struct alignas(16) Node
{
    glm::vec4 boundsMin = glm::vec4(1e30f);
    glm::vec4 boundsMax = glm::vec4(-1e30f);
    uint32_t triIndex = 0;
    uint32_t numTris = 0;
    uint32_t childrenIndex = 0;

    void GrowBounds(Triangle tri);

private:
    int pad[1];
};

struct Mesh
{
    std::vector<glm::vec4> vertices;
    std::vector<Triangle> tris;

    std::vector<Node> nodes;
    
    Mesh(const char* filePath, std::vector<Material>& materials);

private:
    GLuint meshSSBO, bvhSSBO;

    std::vector<std::string> materialNames;

    unsigned int usedNodes = 1;

    void Load(const char* filePath, std::vector<Material>& materials);
    void LoadMtl(const char* filePath, std::vector<Material>& materials);
    void GenBoundingBox();
    void SplitNode(unsigned int nodeIndex, unsigned int depth);
};