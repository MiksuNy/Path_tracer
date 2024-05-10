#include <iostream>

#include <glm.hpp>

#include "Utility.h"
#include "Shader.h"
#include "Camera.h"

#define SCREEN_W 1920
#define SCREEN_H 1080

const float screenQuadVertices[] = {
    -1.0f, -1.0f, 0.0f,
     1.0f, -1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
};

const float screenQuadTexCoords[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f
};

const unsigned int screenQuadIndices[] = {
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

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(Error::MessageCallback, 0);

    glViewport(0, 0, SCREEN_W, SCREEN_H);



    Shader rayTraceVertex, rayTraceFragment;
    Program mainProgram;

    Shader accumVertex, accumFragment;
    Program accumProgram;

    rayTraceVertex.Create(GL_VERTEX_SHADER);
    rayTraceVertex.Parse("src/res/shaders/rt_vertex.vert");
    rayTraceFragment.Create(GL_FRAGMENT_SHADER);
    rayTraceFragment.Parse("src/res/shaders/rt_fragment.frag");

    mainProgram.Create();
    mainProgram.Attach(rayTraceVertex);
    mainProgram.Attach(rayTraceFragment);
    mainProgram.Link();

    accumVertex.Create(GL_VERTEX_SHADER);
    accumVertex.Parse("src/res/shaders/accum_vertex.vert");
    accumFragment.Create(GL_FRAGMENT_SHADER);
    accumFragment.Parse("src/res/shaders/accum_fragment.frag");

    accumProgram.Create();
    accumProgram.Attach(accumVertex);
    accumProgram.Attach(accumFragment);
    accumProgram.Link();


    
    GLuint vao, vbo, ebo;

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



    Camera cam(glm::vec3(0, 0, 1));

    Object::Material metal1;
    metal1.baseColor = glm::vec3(0.1, 1.0, 0.1);
    metal1.emissionColor = glm::vec3(0);
    metal1.emissionStrength = 0.0f;
    metal1.roughness = 0.01f;

    Object::Material diffuse1;
    diffuse1.baseColor = glm::vec3(0.1, 0.1, 1.0);
    diffuse1.emissionColor = glm::vec3(0);
    diffuse1.emissionStrength = 0.0f;
    diffuse1.roughness = 1.0f;

    Object::Material glass1;
    glass1.baseColor = glm::vec3(1.0, 0.1, 0.1);
    glass1.emissionColor = glm::vec3(0);
    glass1.emissionStrength = 0.0f;
    glass1.roughness = 0.01f;
    glass1.isRefractive = true;
    glass1.ior = 0.45f;
    glass1.refractionAmount = 0.98f;

    Object::Material ground;
    ground.baseColor = glm::vec3(1);
    ground.emissionColor = glm::vec3(0);
    ground.emissionStrength = 0.0f;
    ground.roughness = 1.0f;

    Object::Material light;
    light.baseColor = glm::vec3(1);
    light.emissionColor = glm::vec3(1);
    light.emissionStrength = 4.0f;
    light.isLight = true;

    mainProgram.Use();

    Object::Sphere sphere1(glm::vec3(-0.6f, 0.3f, -1.0f), 0.3f);
    mainProgram.SetUniform3f("sphere1.position",                    sphere1.position);
    mainProgram.SetUniform1f("sphere1.radius",                      sphere1.radius);
    mainProgram.SetUniform3f("sphere1.material.baseColor",          metal1.baseColor);
    mainProgram.SetUniform1f("sphere1.material.roughness",          metal1.roughness);
    mainProgram.SetUniform3f("sphere1.material.emissionColor",      metal1.emissionColor);
    mainProgram.SetUniform1f("sphere1.material.emissionStrength",   metal1.emissionStrength);
    mainProgram.SetUniform1i("sphere1.material.isRefractive",       metal1.isRefractive);
    mainProgram.SetUniform1f("sphere1.material.ior",                metal1.ior);
    mainProgram.SetUniform1f("sphere1.material.refractionAmount",   metal1.refractionAmount);

    Object::Sphere sphere2(glm::vec3(0.0f, 0.3f, -1.0f), 0.3f);
    mainProgram.SetUniform3f("sphere2.position",                    sphere2.position);
    mainProgram.SetUniform1f("sphere2.radius",                      sphere2.radius);
    mainProgram.SetUniform3f("sphere2.material.baseColor",          diffuse1.baseColor);
    mainProgram.SetUniform1f("sphere2.material.roughness",          diffuse1.roughness);
    mainProgram.SetUniform3f("sphere2.material.emissionColor",      diffuse1.emissionColor);
    mainProgram.SetUniform1f("sphere2.material.emissionStrength",   diffuse1.emissionStrength);
    mainProgram.SetUniform1i("sphere2.material.isRefractive",       diffuse1.isRefractive);
    mainProgram.SetUniform1f("sphere2.material.ior",                diffuse1.ior);
    mainProgram.SetUniform1f("sphere2.material.refractionAmount",   diffuse1.refractionAmount);

    Object::Sphere sphere3(glm::vec3(0.6f, 0.3f, -1.0f), 0.3f);
    mainProgram.SetUniform3f("sphere3.position",                    sphere3.position);
    mainProgram.SetUniform1f("sphere3.radius",                      sphere3.radius);
    mainProgram.SetUniform3f("sphere3.material.baseColor",          glass1.baseColor);
    mainProgram.SetUniform1f("sphere3.material.roughness",          glass1.roughness);
    mainProgram.SetUniform3f("sphere3.material.emissionColor",      glass1.emissionColor);
    mainProgram.SetUniform1f("sphere3.material.emissionStrength",   glass1.emissionStrength);
    mainProgram.SetUniform1i("sphere3.material.isRefractive",       glass1.isRefractive);
    mainProgram.SetUniform1f("sphere3.material.ior",                glass1.ior);
    mainProgram.SetUniform1f("sphere3.material.refractionAmount",   glass1.refractionAmount);

    Object::Sphere groundSphere(glm::vec3(0.0f, -1000.0f, -1.0f), 1000.0f);
    mainProgram.SetUniform3f("ground.position",                     groundSphere.position);
    mainProgram.SetUniform1f("ground.radius",                       groundSphere.radius);
    mainProgram.SetUniform3f("ground.material.baseColor",           ground.baseColor);
    mainProgram.SetUniform1f("ground.material.roughness",           ground.roughness);
    mainProgram.SetUniform3f("ground.material.emissionColor",       ground.emissionColor);
    mainProgram.SetUniform1f("ground.material.emissionStrength",    ground.emissionStrength);
    mainProgram.SetUniform1i("ground.material.isRefractive",        ground.isRefractive);
    mainProgram.SetUniform1f("ground.material.ior",                 ground.ior);

    Object::Sphere lightSphere(glm::vec3(500.0f, 500.0f, -1.0f), 350.0f);
    mainProgram.SetUniform3f("light.position",                     lightSphere.position);
    mainProgram.SetUniform1f("light.radius",                       lightSphere.radius);
    mainProgram.SetUniform3f("light.material.baseColor",           light.baseColor);
    mainProgram.SetUniform1f("light.material.roughness",           light.roughness);
    mainProgram.SetUniform3f("light.material.emissionColor",       light.emissionColor);
    mainProgram.SetUniform1f("light.material.emissionStrength",    light.emissionStrength);
    mainProgram.SetUniform1i("light.material.isRefractive",        light.isRefractive);
    mainProgram.SetUniform1f("light.material.ior",                 light.ior);

    mainProgram.Unuse();

    double prevFrameTime = 0.0;
    double currFrameTime = 0.0;
    
    float currAccumPass = 0.0f;

    glBindVertexArray(vao);
    
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        prevFrameTime = glfwGetTime();

        //if (glfwGetKey(window, GLFW_KEY_Q)) lightSphere.position.x -= 0.01f;
        //if (glfwGetKey(window, GLFW_KEY_E)) lightSphere.position.x += 0.01f;
        //mainProgram.SetUniform3f("light.position", lightSphere.position);
        //std::cout << cam.velocity.x << ", " << cam.velocity.y << ", " << cam.velocity.z << std::endl;
        if (abs(cam.velocity.x) > 0.001 || abs(cam.velocity.y) > 0.001 || abs(cam.velocity.z) > 0.001)
        {
            currAccumPass = 0.0f;

            glBindFramebuffer(GL_FRAMEBUFFER, screenFboID);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        if (glfwGetKey(window, GLFW_KEY_W)) cam.velocity.y += 0.0025f;
        if (glfwGetKey(window, GLFW_KEY_S)) cam.velocity.y -= 0.0025f;
        if (glfwGetKey(window, GLFW_KEY_A)) cam.velocity.x -= 0.0025f;
        if (glfwGetKey(window, GLFW_KEY_D)) cam.velocity.x += 0.0025f;
        if (glfwGetKey(window, GLFW_KEY_UP))
        {
            cam.position.z -=   0.01f;
            cam.forward -=      0.01f;

            currAccumPass = 0.0f;

            glBindFramebuffer(GL_FRAMEBUFFER, screenFboID);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN))
        {
            cam.position.z +=   0.01f;
            cam.forward +=      0.01f;

            currAccumPass = 0.0f;

            glBindFramebuffer(GL_FRAMEBUFFER, screenFboID);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        cam.velocity *= 0.8f;
        cam.position += cam.velocity;

        mainProgram.Use();
        mainProgram.SetUniformCamera(cam);
        mainProgram.Unuse();

        /* Render here */
        mainProgram.Use();
        glBindFramebuffer(GL_FRAMEBUFFER, screenFboID);
        glBindTexture(GL_TEXTURE_2D, accumTexID);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenQuadIndices);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        mainProgram.Unuse();

        mainProgram.Use();
        currAccumPass += 1.0f;
        std::cout << currAccumPass << std::endl;
        mainProgram.SetUniform1f("currAccumPass", currAccumPass);
        mainProgram.Unuse();

        accumProgram.Use();
        glBindTexture(GL_TEXTURE_2D, accumTexID);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenQuadIndices);
        glBindTexture(GL_TEXTURE_2D, 0);
        accumProgram.Unuse();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

        currFrameTime = glfwGetTime();
        double deltaTime = currFrameTime - prevFrameTime;
        mainProgram.Use();
        mainProgram.SetUniform1f("frameTime", deltaTime);
        mainProgram.Unuse();
        std::cerr << "Frametime: " << deltaTime << " seconds" << std::endl;
    }

    glfwTerminate();
    return 0;
}