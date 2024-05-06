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

        float ior = 0.0f;
        float refractionAmount = 0.0f;
        bool isRefractive = false;

        bool isLight = false;
    };

    class Sphere
    {
    public:
        glm::vec3 position;
        float radius;

        Sphere(glm::vec3 pos, float rad);
    };

    class Plane
    {
    public:
        glm::vec3 position;

        Plane(glm::vec3 pos);
    };

    class Cube
    {
    public:
        glm::vec3 position;
        float t1, t2, t3;
        
        Cube(glm::vec3 pos);
    };
};