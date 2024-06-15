#pragma once

#include <vector>
#include <glm.hpp>

#include "Utility.h"

#include "Shader.h"

struct Material
{
    glm::vec3 baseColor = glm::vec3(0);
    float roughness = 0.0f;

    glm::vec3 emissionColor = glm::vec3(0);
    float emissionStrength = 0.0f;
    bool isLight = false;

    float ior = 0.0f;
    float refractionAmount = 0.0f;
    bool isRefractive = false;
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
    glm::vec3 p1, p2, p3;

    Triangle(struct Program& program, const char* name, glm::vec3 _p1, glm::vec3 _p2, glm::vec3 _p3, struct Material& material);
};

struct Mesh
{
public:
    std::vector<float> vertices;
    std::vector<int> indices;

    Mesh(const char* filePath);

private:
    void Load(const char* filePath);
};