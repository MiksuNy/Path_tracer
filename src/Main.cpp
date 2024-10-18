#include <iostream>
#include <string>

#include <glm.hpp>
#include <gtx/rotate_vector.hpp>

#include "Renderer.h"

int main()
{
    Renderer::Init();

    ShaderProgram rtProgram("res/shaders/rt_vertex.vert", "res/shaders/rt_fragment.frag");
    ShaderProgram accumProgram("res/shaders/accum_vertex.vert", "res/shaders/accum_fragment.frag");
    ComputeProgram computeProgram("res/shaders/rt_compute.comp");

    Material specular;
    specular.baseColor = glm::vec4(0.9, 0.4, 0.1, 1.0);
    specular.smoothness = 0.98f;
    specular.specularSmoothness = 0.9f;
    specular.specularColor = glm::vec4(1.0);
    specular.specularChance = 0.0f;
    specular.emissionColor = glm::vec4(0);
    specular.emissionStrength = 0.0f;
    specular.refractionAmount = 0.0f;
    specular.ior = 1.470f;
    Renderer::scene.materials.push_back(specular);

    Material diffuse;
    diffuse.baseColor = glm::vec4(0.9, 0.1, 0.1, 1.0);
    diffuse.smoothness = 0.2f;
    diffuse.specularSmoothness = 0.99f;
    diffuse.specularColor = glm::vec4(1.0);
    diffuse.specularChance = 0.1f;
    diffuse.emissionColor = glm::vec4(0);
    diffuse.emissionStrength = 0.0f;
    diffuse.refractionAmount = 0.0f;
    diffuse.ior = 1.4f;
    Renderer::scene.materials.push_back(diffuse);

    Material glass;
    glass.baseColor = glm::vec4(0.2, 0.7, 0.3, 1.0);
    glass.smoothness = 0.99f;
    glass.specularSmoothness = 0.99f;
    glass.specularColor = glm::vec4(1.0);
    glass.specularChance = 0.2f;
    glass.emissionColor = glm::vec4(0);
    glass.emissionStrength = 0.0f;
    glass.refractionAmount = 0.99f;
    glass.ior = 1.15f;
    Renderer::scene.materials.push_back(glass);
    
    Material ground;
    ground.baseColor = glm::vec4(0.9, 0.9, 0.9, 1.0);
    ground.smoothness = 0.1f;
    ground.specularSmoothness = 0.1f;
    ground.specularColor = glm::vec4(1.0);
    ground.specularChance = 0.0f;
    ground.emissionColor = glm::vec4(0);
    ground.emissionStrength = 0.0f;
    ground.refractionAmount = 0.0f;
    ground.ior = 1.5f;
    Renderer::scene.materials.push_back(ground);

    Material sun;
    sun.baseColor = glm::vec4(1.0);
    sun.emissionColor = glm::vec4(1.0);
    sun.emissionStrength = 2.0f;
    Renderer::scene.materials.push_back(sun);

    Mesh mesh("res/meshes/bunny.obj", 3);

    Triangle(Renderer::scene, glm::vec3(5000.0, 0.0, 5000.0), glm::vec3(-5000.0, 0.0, 5000.0), glm::vec3(0.0, 0.0, -5000.0), 3);

    for (int i = -2; i < 2; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            for (int k = -2; k < 2; k++)
            {
                Sphere(Renderer::scene, glm::vec3((float)i, ((float)j) + 0.501, (float)k), 0.5, rand() % 5);
            }
        }
    }

    //Sphere(Renderer::scene, glm::vec3(0.0, 2.0, 0.0), 2.0, 3);
    //Sphere(Renderer::scene, glm::vec3(5000.0, 0.0, 0.0), 50.0, 4);

    //Sphere(Renderer::scene, glm::vec3(-0.2, 1.0, 0.0), 0.5, 0);
    //Sphere(Renderer::scene, glm::vec3(2.5, 2.5, 0.0), 2.5, 3);
    //Sphere(Renderer::scene, glm::vec3(100.0, 100.0, 0.0), 5.0, 4);

    Renderer::scene.SetupSSBOs();

    double prevFrameTime = 0.0;
    double currFrameTime = 0.0;
    double deltaTime = 0.0;
    
    int currAccumPass = 0;

    bool keyPressed = false;

    bool debugNormal = false;

    glBindVertexArray(Renderer::vao);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(Renderer::window))
    {
        if (Renderer::camera.moving)
        {
            currAccumPass = 0;

            glBindFramebuffer(GL_FRAMEBUFFER, Renderer::rtFboID);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            Renderer::camera.moving = false;
        }

        Renderer::camera.ProcessInput(Renderer::window, deltaTime);

        Renderer::camera.UpdateView();
        rtProgram.Use();
        rtProgram.SetUniformCamera(Renderer::camera);
        rtProgram.Unuse();
        
        if (glfwGetKey(Renderer::window, GLFW_KEY_N))
        {
            if (!keyPressed)
            {
                debugNormal = !debugNormal;
                rtProgram.Use();
                glUniform1i(glGetUniformLocation(rtProgram.ID, "debugNormal"), debugNormal);
                rtProgram.Unuse();
            }
            keyPressed = true;
        }
        else
        {
            keyPressed = false;
        }

        // Raytrace the scene
        rtProgram.Use();
        glBindFramebuffer(GL_FRAMEBUFFER, Renderer::rtFboID);
        glBindTexture(GL_TEXTURE_2D, Renderer::accumTexID);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenQuadIndices);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        rtProgram.Unuse();
        
        // Accumulate from previous frame
        accumProgram.Use();
        glBindTexture(GL_TEXTURE_2D, Renderer::accumTexID);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenQuadIndices);
        glBindTexture(GL_TEXTURE_2D, 0);
        accumProgram.Unuse();
        
        currAccumPass++;
        rtProgram.Use();
        glUniform1i(glGetUniformLocation(rtProgram.ID, "currAccumPass"), currAccumPass);
        rtProgram.Unuse();

        /* Swap front and back buffers */
        glfwSwapBuffers(Renderer::window);

        /* Poll for and process events */
        glfwPollEvents();

        currFrameTime = glfwGetTime();
        deltaTime = currFrameTime - prevFrameTime;

        std::string frameTime = std::to_string(deltaTime * 1000.0);
        std::string title = "GLSL Raytracer | Frametime: " + frameTime + " ms" + " | Samples: " + std::to_string(currAccumPass);
        glfwSetWindowTitle(Renderer::window, title.c_str());
        prevFrameTime = currFrameTime;
    }

    glfwTerminate();
    return 0;
}