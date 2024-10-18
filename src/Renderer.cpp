#include "Renderer.h"

namespace Renderer
{
    GLuint vao, vbo, ebo;
    GLuint accumTexID, rtFboID, screenDepthRbID;

    GLFWwindow* window;
    GLFWmonitor* monitor;
    int screenWidth;
    int screenHeight;

    Camera camera;
    Scene scene;
}

int Renderer::Init()
{
    /*
      @@@@    @@                  @           @@  @@
     @@@@@@   @@                 @@          @@@ @@@
     @@  @@@  @@                 @@          @@  @@
    @@    @@  @@          @@@   @@@@ @@  @@ @@@@@@@@
    @@        @@         @@@@@  @@@@ @@  @@ @@@@@@@@
    @@  @@@@  @@         @@  @   @@  @@  @@  @@  @@
    @@  @@@@  @@         @@@     @@  @@  @@  @@  @@
    @@    @@  @@           @@@   @@  @@  @@  @@  @@
     @@   @@  @@         @  @@   @@  @@  @@  @@  @@
     @@@@@@@  @@@@@@@    @@@@@   @@@ @@@@@@  @@  @@
      @@@@    @@@@@@@     @@@    @@@  @@ @@  @@  @@
    */

    glfwSetErrorCallback(Error::ErrorCallback);

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    monitor = glfwGetPrimaryMonitor();
    glfwGetMonitorWorkarea(monitor, NULL, NULL, &screenWidth, &screenHeight); // If we want to use max available resolution

    std::cout << "Screen width: " << screenWidth << ", height: " << screenHeight << "\n";

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(screenWidth, screenHeight, "GLSL Ray Tracer", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW could not be initialized" << std::endl;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(Error::MessageCallback, 0);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, MouseCallback);

    glViewport(0, 0, screenWidth, screenHeight);

    glfwSwapInterval(0); // 0 = no framerate cap, 1 = set framerate cap to monitor refresh rate

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(float), screenQuadVertices);
    glBufferSubData(GL_ARRAY_BUFFER, 12 * sizeof(float), 8 * sizeof(float), screenQuadTexCoords);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), screenQuadIndices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(12 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /*
    @@@@@@@@                                                                            @@@@ @@@@
    @@@@@@@@                  @                                          @             @@@@@@@@@@
       @@                    @@                                         @@             @@   @@
       @@                    @@                                         @@             @@   @@
       @@     @@@@  @@@   @@@@@@@  @@    @@ @@ @@   @@@@       @@@@@   @@@@@ @@    @@ @@@@@@@@@@
       @@    @@@@@@  @@   @@@@@@@  @@    @@ @@@@@  @@@@@@     @@@@@@@  @@@@@ @@    @@ @@@@@@@@@@
       @@   @@@  @@   @@ @@  @@    @@    @@ @@    @@@  @@     @@   @@   @@   @@    @@  @@   @@
       @@   @@    @@  @@@@@  @@    @@    @@ @@    @@    @@    @@@       @@   @@    @@  @@   @@
       @@   @@@@@@@@   @@@   @@    @@    @@ @@    @@@@@@@@    @@@@@@    @@   @@    @@  @@   @@
       @@   @@@@@@@@   @@@   @@    @@    @@ @@    @@@@@@@@     @@@@@@   @@   @@    @@  @@   @@
       @@   @@        @@@@@  @@    @@    @@ @@    @@              @@@   @@   @@    @@  @@   @@
       @@   @@@   @@  @@ @@  @@    @@   @@@ @@    @@@   @@    @@   @@   @@   @@   @@@  @@   @@
       @@    @@@@@@  @@   @@ @@@@  @@@@@@@@ @@     @@@@@@     @@@@@@@   @@@@ @@@@@@@@  @@   @@
       @@     @@@@  @@@   @@@ @@@   @@@@ @@ @@      @@@@       @@@@@     @@@  @@@@ @@  @@   @@
    */

    glGenFramebuffers(1, &rtFboID);
    glBindFramebuffer(GL_FRAMEBUFFER, rtFboID);

    glGenTextures(1, &accumTexID);
    glBindTexture(GL_TEXTURE_2D, accumTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTexID, 0);

    glGenRenderbuffers(1, &screenDepthRbID);
    glBindRenderbuffer(GL_RENDERBUFFER, screenDepthRbID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screenDepthRbID);

    //glEnable(GL_FRAMEBUFFER_SRGB);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    float xOffset = xpos - camera.mouseLastX;
    float yOffset = camera.mouseLastY - ypos;
    camera.mouseLastX = xpos;
    camera.mouseLastY = ypos;

    if (abs(xOffset) > 0 || abs(yOffset) > 0) camera.moving = true;

    float sensitivity = 0.1f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    camera.yaw += xOffset;
    camera.pitch -= yOffset;

    if (camera.pitch > 89.0f) camera.pitch = 89.0f;
    if (camera.pitch < -89.0f) camera.pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    direction.y = sin(glm::radians(camera.pitch));
    direction.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    camera.forward = glm::normalize(direction);
}