#pragma once

#include <vector>
#include <glm.hpp>

#include "Utility.h"

#include "Shader.h"

struct Material
{
public:
    glm::vec4 baseColor;
    glm::vec4 specularColor;
    glm::vec4 emissionColor;
    float smoothness;
    float specularSmoothness;
    float emissionStrength;
    float ior;
    float refractionAmount;
    float specularChance;
private:
    float pad[2];
};

struct Scene
{
public:
    std::vector<Material> materials;
};

struct Sphere
{
public:
    glm::vec3 position;
    float radius;

    Sphere(struct Program& program, const char* name, glm::vec3 pos, float rad, struct Material& material);
};

struct Triangle
{
public:
    glm::vec4 p1, p2, p3;
    uint32_t materialIndex; // Index to scene.materials
    float pad[3];

    Triangle();
    Triangle(struct Program& program, const char* name, glm::vec3 _p1, glm::vec3 _p2, glm::vec3 _p3, uint32_t materialIndex);
    glm::vec3 Center();
};

struct Node
{
    float boundsMin[4] = { 1e30f };
    float boundsMax[4] = { -1e30f };
    bool isLeaf = false;
    int nextIndex = 0;
    int numTris = 0;
};

struct Mesh
{
public:
    std::vector<glm::vec4> vertices;
    std::vector<glm::ivec4> indices;
    std::vector<Triangle> tris;

    uint32_t materialIndex;

    std::vector<Node> nodes;
    
    Mesh(Scene scene, const char* filePath, uint32_t materialIndex);

private:
    void Load(const char* filePath);
    void GenBoundingBox();
};