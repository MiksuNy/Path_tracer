#include "Object.h"

Object::Sphere::Sphere(glm::vec3 pos, float rad)
{
    position = pos;
    radius = rad;
}

Object::Plane::Plane(glm::vec3 pos)
{
    position = pos;
}

Object::Cube::Cube(glm::vec3 pos)
{
    position = pos;
}