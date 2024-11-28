#pragma once

#include <vector>
#include <glm.hpp>

#include "Shader.h"

// TODO: SSBOs need to be padded to vec4 leaving an unused fourth component, is there a way to avoid this?

// Needs to be padded to multiple of 16 bytes for SSBO usage
struct alignas(16) Material
{
    glm::vec4 baseColor = glm::vec4(1);
    glm::vec4 coatColor = glm::vec4(1);
    glm::vec4 emissionColor = glm::vec4(1);
    float smoothness = 0.0f;
    float coatSmoothness = 0.0f;
    float emissionStrength = 0.0f;
    float ior = 1.5f;
    float refractionAmount = 0.0f;
    float coatChance = 0.0f;

private:
    int pad[2];
};

// Needs to be padded to multiple of 16 bytes for SSBO usage
struct alignas(16) Sphere
{
    glm::vec3 position = glm::vec3(0);
    float radius = 0.0f;
    unsigned int materialIndex = 0;
    
    Sphere(struct Scene& scene, glm::vec3 pos, float rad, unsigned int materialIndex);

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

    Triangle();
    Triangle(struct Scene& scene, glm::vec3 _p1, glm::vec3 _p2, glm::vec3 _p3, unsigned int materialIndex);
    glm::vec3 Center();

private:
    int pad[3];
};

struct Scene
{
    std::vector<Material> materials;
    std::vector<Sphere> spheres;
    std::vector<Triangle> triangles;

    void SetupSSBOs();
    void UpdateSSBOs();

private:
    GLuint materialSSBO, sphereSSBO, triangleSSBO;
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
    std::vector<glm::uvec4> indices;
    std::vector<Triangle> tris;
    uint32_t materialIndex = 0;

    std::vector<Node> nodes;
    
    Mesh(const char* filePath, uint32_t materialIndex);

private:
    GLuint meshSSBO, bvhSSBO;

    unsigned int usedNodes = 1;

    void Load(const char* filePath);
    void GenBoundingBox();
    void SplitNode(unsigned int nodeIndex, unsigned int depth);
};