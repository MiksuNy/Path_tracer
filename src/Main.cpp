#include <iostream>

#include <glm.hpp>

#include "Utility.h"
#include "Shader.h"
#include "Camera.h"

#define SCREEN_W 1920
#define SCREEN_H 1080

float screenQuadVertices[] = {
    -1.0f, -1.0f, 0.0f,
     1.0f, -1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,

    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f
};

unsigned int screenQuadIndices[] = {
    0, 1, 2,
    2, 3, 0
};

int main(void)
{
    glfwSetErrorCallback(Error::ErrorCallback);

    /* Initialize the library */
    if (!glfwInit())
    {
        return -1;
    }

    GLFWwindow* window;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(SCREEN_W, SCREEN_H, "GLSL Ray Tracer", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW could not be initialized" << std::endl;
    }

    std::cout << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(Error::MessageCallback, 0);



    Shader rayTraceVertex, rayTraceFragment;
    Program mainProgram;

    rayTraceVertex.Create(GL_VERTEX_SHADER);
    rayTraceVertex.Parse("src/res/shaders/rt_vertex.vert");
    rayTraceFragment.Create(GL_FRAGMENT_SHADER);
    rayTraceFragment.Parse("src/res/shaders/rt_fragment.frag");

    mainProgram.Create();
    mainProgram.Attach(rayTraceVertex);
    mainProgram.Attach(rayTraceFragment);
    mainProgram.Link();


    
    GLuint vao, vbo, ebo;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuadVertices), 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(float), screenQuadVertices);
    glBufferSubData(GL_ARRAY_BUFFER, 12 * sizeof(float), 8 * sizeof(float), screenQuadVertices);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), screenQuadIndices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);



    GLuint accumTexID, screenFboID, screenDepthRbID;

    glGenFramebuffers(1, &screenFboID);
    glBindFramebuffer(GL_FRAMEBUFFER, screenFboID);

    glGenTextures(1, &accumTexID);
    glBindTexture(GL_TEXTURE_2D, accumTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_W, SCREEN_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTexID, 0);

    glGenRenderbuffers(1, &screenDepthRbID);
    glBindRenderbuffer(GL_RENDERBUFFER, screenDepthRbID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_W, SCREEN_H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screenDepthRbID);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);



    Camera cam(glm::vec3(0, 0, 1), 0.8f);

    Object::Material metal1;
    metal1.baseColor = glm::vec3(0.8, 0.2, 0.2);
    metal1.emissionColor = glm::vec3(0);
    metal1.emissionStrength = 0.0f;
    metal1.roughness = 0.05f;

    Object::Material diffuse1;
    diffuse1.baseColor = glm::vec3(0.2, 0.8, 0.2);
    diffuse1.emissionColor = glm::vec3(0);
    diffuse1.emissionStrength = 0.0f;
    diffuse1.roughness = 0.99f;

    Object::Material glass1;
    glass1.baseColor = glm::vec3(1);
    glass1.emissionColor = glm::vec3(0);
    glass1.emissionStrength = 0.0f;
    glass1.roughness = 0.02f;
    glass1.isRefractive = true;
    glass1.ior = 0.12f;
    glass1.refractionAmount = 0.95f;

    Object::Material ground;
    ground.baseColor = glm::vec3(0.9);
    ground.emissionColor = glm::vec3(0);
    ground.emissionStrength = 0.0f;
    ground.roughness = 0.99f;

    Object::Material light;
    light.baseColor = glm::vec3(1);
    light.emissionColor = glm::vec3(1);
    light.emissionStrength = 40.0f;
    light.isLight = true;

    mainProgram.Use();

    Object::Sphere sphere1(glm::vec3(0.0f, 0.1f, -2.0f), 0.1f);
    mainProgram.SetUniform3f("sphere1.position",                    sphere1.position);
    mainProgram.SetUniform1f("sphere1.radius",                      sphere1.radius);
    mainProgram.SetUniform3f("sphere1.material.baseColor",          metal1.baseColor);
    mainProgram.SetUniform1f("sphere1.material.roughness",          metal1.roughness);
    mainProgram.SetUniform3f("sphere1.material.emissionColor",      metal1.emissionColor);
    mainProgram.SetUniform1f("sphere1.material.emissionStrength",   metal1.emissionStrength);
    mainProgram.SetUniform1i("sphere1.material.isRefractive",       metal1.isRefractive);
    mainProgram.SetUniform1f("sphere1.material.ior",                metal1.ior);
    mainProgram.SetUniform1f("sphere1.material.refractionAmount",   metal1.refractionAmount);

    Object::Sphere sphere2(glm::vec3(0.0f, 0.3f, -2.0f), 0.1f);
    mainProgram.SetUniform3f("sphere2.position",                    sphere2.position);
    mainProgram.SetUniform1f("sphere2.radius",                      sphere2.radius);
    mainProgram.SetUniform3f("sphere2.material.baseColor",          metal1.baseColor);
    mainProgram.SetUniform1f("sphere2.material.roughness",          metal1.roughness);
    mainProgram.SetUniform3f("sphere2.material.emissionColor",      metal1.emissionColor);
    mainProgram.SetUniform1f("sphere2.material.emissionStrength",   metal1.emissionStrength);
    mainProgram.SetUniform1i("sphere2.material.isRefractive",       metal1.isRefractive);
    mainProgram.SetUniform1f("sphere2.material.ior",                metal1.ior);
    mainProgram.SetUniform1f("sphere2.material.refractionAmount",   metal1.refractionAmount);

    Object::Sphere groundSphere(glm::vec3(0.0f, -1000.0f, -2.0f), 1000.0f);
    mainProgram.SetUniform3f("ground.position",                     groundSphere.position);
    mainProgram.SetUniform1f("ground.radius",                       groundSphere.radius);
    mainProgram.SetUniform3f("ground.material.baseColor",           ground.baseColor);
    mainProgram.SetUniform1f("ground.material.roughness",           ground.roughness);
    mainProgram.SetUniform3f("ground.material.emissionColor",       ground.emissionColor);
    mainProgram.SetUniform1f("ground.material.emissionStrength",    ground.emissionStrength);
    mainProgram.SetUniform1i("ground.material.isRefractive",        ground.isRefractive);
    mainProgram.SetUniform1f("ground.material.ior",                 ground.ior);

    Object::Sphere lightSphere(glm::vec3(2000.0f, 2000.0f, -2.0f), 800.0f);
    mainProgram.SetUniform3f("light.position",                     lightSphere.position);
    mainProgram.SetUniform1f("light.radius",                       lightSphere.radius);
    mainProgram.SetUniform3f("light.material.baseColor",           light.baseColor);
    mainProgram.SetUniform1f("light.material.roughness",           light.roughness);
    mainProgram.SetUniform3f("light.material.emissionColor",       light.emissionColor);
    mainProgram.SetUniform1f("light.material.emissionStrength",    light.emissionStrength);
    mainProgram.SetUniform1i("light.material.isRefractive",        light.isRefractive);
    mainProgram.SetUniform1f("light.material.ior",                 light.ior);



    double prevFrameTime = 0.0;
    double currFrameTime = 0.0;
    
    glBindVertexArray(vao);
    
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        prevFrameTime = glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_Q)) sphere1.position.y -= 0.001f;
        if (glfwGetKey(window, GLFW_KEY_E)) sphere1.position.y += 0.001f;
        mainProgram.SetUniform3f("sphere1.position", sphere1.position);

        if (glfwGetKey(window, GLFW_KEY_W)) cam.position.y += 0.01f;
        if (glfwGetKey(window, GLFW_KEY_S)) cam.position.y -= 0.01f;
        if (glfwGetKey(window, GLFW_KEY_A)) cam.position.x -= 0.01f;
        if (glfwGetKey(window, GLFW_KEY_D)) cam.position.x += 0.01f;
        if (glfwGetKey(window, GLFW_KEY_UP))
        {
            cam.position.z -= 0.02f;
            cam.forward -= 0.02f;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN))
        {
            cam.position.z += 0.02f;
            cam.forward += 0.02f;
        }

        mainProgram.SetUniformCamera(cam);

        /* Render here */
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenQuadIndices);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

        currFrameTime = glfwGetTime();
        double deltaTime = currFrameTime - prevFrameTime;
        std::cerr << "Frametime: " << deltaTime << " seconds" << std::endl;
    }

    rayTraceVertex.~Shader();
    rayTraceFragment.~Shader();
    mainProgram.~Program();

    glfwTerminate();
    return 0;
}