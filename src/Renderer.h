#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Error.h"
#include "Camera.h"
#include "Object.h"
#include "Shader.h"

constexpr float screenQuadVertices[] = {
    -1.0f, -1.0f, 0.0f,
     1.0f, -1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
};

constexpr float screenQuadTexCoords[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f
};

constexpr unsigned int screenQuadIndices[] = {
    0, 1, 2,
    2, 3, 0
};

namespace Renderer
{
    extern GLuint vao, vbo, ebo;
    extern GLuint accumTexID, rtFboID, screenDepthRbID;

    extern GLFWwindow* window;
    extern GLFWmonitor* monitor;
    extern int screenWidth;
    extern int screenHeight;

    extern Camera camera;
    extern Scene scene;

    int Init(int width, int height);
    void MouseCallback(GLFWwindow* window, double xpos, double ypos);
}