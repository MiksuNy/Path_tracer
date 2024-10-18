#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

struct Error
{
    static void GLAPIENTRY
        MessageCallback(GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar* message,
            const void* userParam)
    {
        fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
    }

    static void ErrorCallback(int error, const char* description)
    {
        fprintf(stderr, "Error: %s\n", description);
    }

    static void GetError()
    {
        GLenum err = glGetError();
        std::cout << err << ", " << glewGetErrorString(err) << std::endl;
    }
};