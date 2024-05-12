#include "Object.h"

Object::Sphere::Sphere(glm::vec3 pos, float rad)
{
    position = pos;
    radius = rad;
}

Object::Triangle::Triangle(glm::vec3 _p1, glm::vec3 _p2, glm::vec3 _p3)
{
    p1 = _p1;
    p2 = _p2;
    p3 = _p3;
}