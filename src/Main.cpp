#include <iostream>

#include <glm.hpp>

#include "Utility.h"
#include "Shader.h"
#include "Camera.h"

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


int main()
{
    glfwSetErrorCallback(Error::ErrorCallback);

    /* Initialize the library */
    if (!glfwInit())
    {
        return -1;
    }

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    GLFWwindow* window;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int SCREEN_W, SCREEN_H;
    glfwGetMonitorWorkarea(monitor, NULL, NULL, &SCREEN_W, &SCREEN_H);

    std::cout << "Screen width: " << SCREEN_W << ", height: " << SCREEN_H << "\n";

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

    glfwSwapInterval(0); // 0 = no framerate cap, 1 = set framerate cap to monitor refresh rate

    Shader rayTraceVertex, rayTraceFragment;
    Program mainProgram;

    Shader accumVertex, accumFragment;
    Program accumProgram;

    rayTraceVertex.Create(GL_VERTEX_SHADER);
    rayTraceVertex.Parse("res/shaders/rt_vertex.vert");
    rayTraceFragment.Create(GL_FRAGMENT_SHADER);
    rayTraceFragment.Parse("res/shaders/rt_fragment.frag");

    mainProgram.Create();
    mainProgram.Attach(rayTraceVertex);
    mainProgram.Attach(rayTraceFragment);
    mainProgram.Link();

    accumVertex.Create(GL_VERTEX_SHADER);
    accumVertex.Parse("res/shaders/accum_vertex.vert");
    accumFragment.Create(GL_FRAGMENT_SHADER);
    accumFragment.Parse("res/shaders/accum_fragment.frag");

    accumProgram.Create();
    accumProgram.Attach(accumVertex);
    accumProgram.Attach(accumFragment);
    accumProgram.Link();

    mainProgram.Use();
    glUniform1i(glGetUniformLocation(mainProgram.ID, "SCREEN_W"), SCREEN_W);
    glUniform1i(glGetUniformLocation(mainProgram.ID, "SCREEN_H"), SCREEN_H);
    mainProgram.Unuse();
    
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTexID, 0);

    glGenRenderbuffers(1, &screenDepthRbID);
    glBindRenderbuffer(GL_RENDERBUFFER, screenDepthRbID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_W, SCREEN_H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screenDepthRbID);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);



    Camera cam(glm::vec3(0, 0, 1));

    Material metal1;
    metal1.baseColor = glm::vec3(1);
    metal1.emissionColor = glm::vec3(0);
    metal1.emissionStrength = 0.0f;
    metal1.roughness = 0.01f;

    Material diffuse1;
    diffuse1.baseColor = glm::vec3(0.1, 0.1, 1.0);
    diffuse1.emissionColor = glm::vec3(0);
    diffuse1.emissionStrength = 0.0f;
    diffuse1.roughness = 1.0f;

    Material diffuse2;
    diffuse2.baseColor = glm::vec3(0.1, 1.0, 0.1);
    diffuse2.emissionColor = glm::vec3(0);
    diffuse2.emissionStrength = 0.0f;
    diffuse2.roughness = 1.0f;

    Material diffuse3;
    diffuse3.baseColor = glm::vec3(1.0, 0.1, 0.1);
    diffuse3.emissionColor = glm::vec3(0);
    diffuse3.emissionStrength = 0.0f;
    diffuse3.roughness = 1.0f;

    Material glass1;
    glass1.baseColor = glm::vec3(1.0, 0.9, 0.7);
    glass1.emissionColor = glm::vec3(0);
    glass1.emissionStrength = 0.0f;
    glass1.roughness = 0.1f;
    glass1.ior = 0.35f;
    glass1.refractionAmount = 0.96f;

    Material ground;
    ground.baseColor = glm::vec3(1);
    ground.emissionColor = glm::vec3(0);
    ground.emissionStrength = 0.0f;
    ground.roughness = 1.0f;

    Material light;
    light.baseColor = glm::vec3(1);
    light.emissionColor = glm::vec3(1);
    light.emissionStrength = 100.0f;

    Material sunLight;
    sunLight.baseColor = glm::vec3(1);
    sunLight.emissionColor = glm::vec3(0.97, 0.86, 0.81);
    sunLight.emissionStrength = 10.0f;

    Mesh mesh("res/meshes/bunny1.obj");

    GLuint vertexSSBO, indexSSBO, aabbSSBO;

    glGenBuffers(1, &vertexSSBO);
    glGenBuffers(1, &indexSSBO);
    glGenBuffers(1, &aabbSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vertexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, mesh.indices.size() * sizeof(int), mesh.indices.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, indexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, aabbSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 8, NULL, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(float), mesh.boundsMin);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(float), 4 * sizeof(float), mesh.boundsMax);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, aabbSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    Triangle(mainProgram, "tri1", glm::vec3(-5000.0, 0.0, 5000.0), glm::vec3(5000.0, 0.0, 5000.0), glm::vec3(0.0, 0.0, -5000.0), ground);
    //Triangle(mainProgram, "tri2", glm::vec3(-1.0, 0.0 - 1000, -3.0), glm::vec3(1.0, 0.0 - 1000, -3.0), glm::vec3(0.0, 3.0 - 1000, -3.0), diffuse1);
    
    //Sphere(mainProgram, "sphere1", glm::vec3(-0.6f, 0.3f, .0f), 0.3f, diffuse1);
    //Sphere(mainProgram, "sphere2", glm::vec3(0.0f, 0.3f, .0f),  0.3f, metal1);
    //Sphere(mainProgram, "sphere3", glm::vec3(0.6f, 0.3f, .0f),  0.3f, glass1);
    //Sphere(mainProgram, "sunSphere", glm::vec3(10000.0f, 10000.0f, -1.0f), 3000.0f, sunLight);

    double prevFrameTime = 0.0;
    double currFrameTime = 0.0;
    int frameCounter = 0;
    
    int currAccumPass = 0;

    glBindVertexArray(vao);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        if (abs(cam.velocity.x) > 0.001 || abs(cam.velocity.y) > 0.001 || abs(cam.velocity.z) > 0.001)
        {
            currAccumPass = 0;

            glBindFramebuffer(GL_FRAMEBUFFER, screenFboID);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        if (glfwGetKey(window, GLFW_KEY_W)) cam.velocity.y += 0.0025f;
        if (glfwGetKey(window, GLFW_KEY_S)) cam.velocity.y -= 0.0025f;
        if (glfwGetKey(window, GLFW_KEY_A)) cam.velocity.x -= 0.0025f;
        if (glfwGetKey(window, GLFW_KEY_D)) cam.velocity.x += 0.0025f;

        if (glfwGetKey(window, GLFW_KEY_UP)) cam.velocity.z -= 0.0025f;
        if (glfwGetKey(window, GLFW_KEY_DOWN)) cam.velocity.z += 0.0025f;

        cam.velocity *= 0.8f;
        cam.position += cam.velocity;
        cam.UpdateView();
        mainProgram.SetUniformCamera(cam);

        /* Render here */
        mainProgram.Use();
        glBindFramebuffer(GL_FRAMEBUFFER, screenFboID);
        glBindTexture(GL_TEXTURE_2D, accumTexID);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenQuadIndices);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        mainProgram.Unuse();

        accumProgram.Use();
        glBindTexture(GL_TEXTURE_2D, accumTexID);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenQuadIndices);
        glBindTexture(GL_TEXTURE_2D, 0);
        accumProgram.Unuse();

        currAccumPass++;
        mainProgram.Use();
        mainProgram.SetUniform1f("currAccumPass", (float)currAccumPass);
        mainProgram.Unuse();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

        currFrameTime = glfwGetTime();
        double deltaTime = currFrameTime - prevFrameTime;
        frameCounter++;
        if (deltaTime >= 1.0 / 30.0)
        {
            std::string fps = std::to_string((1.0 / deltaTime) * frameCounter);
            std::string title = "GLSL Raytracer - FPS: " + fps;
            glfwSetWindowTitle(window, title.c_str());
            prevFrameTime = currFrameTime;
            frameCounter = 0;
        }

        mainProgram.Use();
        mainProgram.SetUniform1f("frameTime", deltaTime);
        mainProgram.Unuse();
    }

    glfwTerminate();
    return 0;
}