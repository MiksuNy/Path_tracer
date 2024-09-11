#include <iostream>

#include <glm.hpp>
#include <gtx/rotate_vector.hpp>

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

    Scene scene;

    Camera cam(glm::vec3(0, 0, 1));



    Material specular;
    specular.baseColor = glm::vec4(0.9, 0.4, 0.1, 1.0);
    specular.smoothness = 0.5f;
    specular.specularSmoothness = 0.9f;
    specular.specularColor = glm::vec4(1.0);
    specular.specularChance = 0.4f;
    specular.emissionColor = glm::vec4(0);
    specular.emissionStrength = 0.0f;
    specular.refractionAmount = 0.0f;
    specular.ior = 1.4f;
    scene.materials.push_back(specular);

    Material diffuse;
    diffuse.baseColor = glm::vec4(0.1, 0.9, 0.1, 1.0);
    diffuse.smoothness = 0.03f;
    diffuse.specularSmoothness = 0.03f;
    diffuse.specularColor = glm::vec4(1.0);
    diffuse.specularChance = 0.0f;
    diffuse.emissionColor = glm::vec4(0);
    diffuse.emissionStrength = 0.0f;
    diffuse.refractionAmount = 0.0f;
    diffuse.ior = 1.4f;
    scene.materials.push_back(diffuse);

    Material glass;
    glass.baseColor = glm::vec4(1.0, 0.7, 0.3, 1.0);
    glass.smoothness = 0.92f;
    glass.specularSmoothness = 0.92f;
    glass.specularColor = glm::vec4(1.0);
    glass.specularChance = 0.0f;
    glass.emissionColor = glm::vec4(0);
    glass.emissionStrength = 0.0f;
    glass.refractionAmount = 0.92f;
    glass.ior = 1.5f;
    scene.materials.push_back(glass);
    
    Material ground;
    ground.baseColor = glm::vec4(0.9, 0.9, 0.9, 1.0);
    ground.smoothness = 0.1f;
    ground.specularSmoothness = 0.0f;
    ground.specularColor = glm::vec4(1.0);
    ground.specularChance = 0.0f;
    ground.emissionColor = glm::vec4(0);
    ground.emissionStrength = 0.0f;
    ground.refractionAmount = 0.0f;
    ground.ior = 1.6f;
    scene.materials.push_back(ground);

    Material sun;
    sun.baseColor = glm::vec4(1.0);
    sun.emissionColor = glm::vec4(1.0);
    sun.emissionStrength = 2.0f;
    scene.materials.push_back(sun);

    Mesh mesh(scene, "res/meshes/bunny1.obj", 0);

    Triangle(mainProgram, "tri1", glm::vec3(-5000.0, 0.0, 5000.0), glm::vec3(5000.0, 0.0, 5000.0), glm::vec3(0.0, 0.0, -5000.0), 3);
    //Sphere(mainProgram, "sphere1", glm::vec3(-0.5, 0.5, 0.0), 0.5, glass);
    Sphere(mainProgram, "sphere2", glm::vec3(100.0, 100.0, 0.0), 5.0, sun);

    double prevFrameTime = 0.0;
    double currFrameTime = 0.0;
    double deltaTime = 0.0;
    
    int currAccumPass = 0;

    glBindVertexArray(vao);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        if (cam.moving)
        {
            currAccumPass = 0;

            glBindFramebuffer(GL_FRAMEBUFFER, screenFboID);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            cam.moving = false;
        }

        if (glfwGetKey(window, GLFW_KEY_W))
        {
            cam.moving = true;
            cam.position -= cam.forward * cam.speed;
        }
        else if (glfwGetKey(window, GLFW_KEY_S))
        {
            cam.moving = true;
            cam.position += cam.forward * cam.speed;
        }
        if (glfwGetKey(window, GLFW_KEY_A))
        {
            cam.moving = true;
            cam.position -= cam.right * cam.speed;
        }
        else if (glfwGetKey(window, GLFW_KEY_D))
        {
            cam.moving = true;
            cam.position += cam.right * cam.speed;
        }
        if (glfwGetKey(window, GLFW_KEY_E))
        {
            cam.moving = true;
            cam.position += cam.up * cam.speed;
        }
        else if (glfwGetKey(window, GLFW_KEY_Q))
        {
            cam.moving = true;
            cam.position -= cam.up * cam.speed;
        }

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
        glUniform1i(glGetUniformLocation(mainProgram.ID, "currAccumPass"), currAccumPass);
        mainProgram.Unuse();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

        currFrameTime = glfwGetTime();
        deltaTime = currFrameTime - prevFrameTime;

        std::string frameTime = std::to_string(deltaTime * 1000.0);
        std::string title = "GLSL Raytracer | Frametime: " + frameTime + " ms" + " | Current sample: " + std::to_string(currAccumPass);
        glfwSetWindowTitle(window, title.c_str());
        prevFrameTime = currFrameTime;

        mainProgram.Use();
        mainProgram.SetUniform1f("frameTime", deltaTime);
        mainProgram.Unuse();
    }

    glfwTerminate();
    return 0;
}