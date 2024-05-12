#pragma once

#include <glm.hpp>

class Object
{
public:
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

    class Sphere
    {
    public:
        glm::vec3 position;
        float radius;

        Sphere(glm::vec3 pos, float rad);
    };

    class Triangle
    {
    public:
        glm::vec3 p1, p2, p3;

        Triangle(glm::vec3 _p1, glm::vec3 _p2, glm::vec3 _p3);
    };
};