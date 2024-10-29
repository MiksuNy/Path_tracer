#include <iostream>
#include <string>

#include <glm.hpp>
#include <gtx/rotate_vector.hpp>

#include "Renderer.h"

int main()
{
    // Sets window size to monitor size if both are null
    Renderer::Init(NULL, NULL);

    //ShaderProgram rtProgram("res/shaders/rt.vert", "res/shaders/rt.frag");
    //ShaderProgram accumProgram("res/shaders/accum.vert", "res/shaders/accum.frag");
    
    ComputeProgram computeProgram("res/shaders/rt.comp");
    ShaderProgram computeAccumProgram("res/shaders/compute_accum.vert", "res/shaders/compute_accum.frag");

    Material specular;
    specular.baseColor = glm::vec4(0.2, 0.9, 0.1, 1.0);
    specular.smoothness = 0.9f;
    specular.specularSmoothness = 0.0f;
    specular.specularColor = glm::vec4(1.0);
    specular.specularChance = 0.0f;
    specular.emissionColor = glm::vec4(0);
    specular.emissionStrength = 0.0f;
    specular.refractionAmount = 0.0f;
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
    Renderer::scene.materials.push_back(diffuse);

    Material glass;
    glass.baseColor = glm::vec4(0.9, 0.1, 0.1, 1.0);
    glass.smoothness = 0.99f;
    glass.specularSmoothness = 0.99f;
    glass.specularColor = glm::vec4(1.0);
    glass.specularChance = 0.2f;
    glass.emissionColor = glm::vec4(0);
    glass.emissionStrength = 0.0f;
    glass.refractionAmount = 0.8f;
    glass.ior = 1.05f;
    Renderer::scene.materials.push_back(glass);
    
    Material ground;
    ground.baseColor = glm::vec4(0.9, 0.9, 0.9, 1.0);
    ground.smoothness = 0.0f;
    ground.specularSmoothness = 0.0f;
    ground.specularColor = glm::vec4(1.0);
    ground.specularChance = 0.0f;
    ground.emissionColor = glm::vec4(0);
    ground.emissionStrength = 0.0f;
    ground.refractionAmount = 0.0f;
    Renderer::scene.materials.push_back(ground);

    Material sun;
    sun.baseColor = glm::vec4(1.0);
    sun.emissionColor = glm::vec4(1.0);
    sun.emissionStrength = 5.0f;
    Renderer::scene.materials.push_back(sun);

    Mesh mesh("res/meshes/bunny1.obj", 3);

    Triangle(Renderer::scene, glm::vec3(-1.0, 0.0, 3.0), glm::vec3(1.0, 0.0, 3.0), glm::vec3(0.0, 1.4, 3.0), 4);
    Triangle(Renderer::scene, glm::vec3(5000.0, 0.0, 5000.0), glm::vec3(-5000.0, 0.0, 5000.0), glm::vec3(0.0, 0.0, -5000.0), 3);

    //for (int i = -6; i < 6; i++)
    //{
    //    for (int j = 0; j < 1; j++)
    //    {
    //        for (int k = -6; k < 6; k++)
    //        {
    //            Sphere(Renderer::scene, glm::vec3(((float)i) / 3.0, ((float)j) + 0.101, ((float)k) / 3.0), 0.1, 2);
    //        }
    //    }
    //}

    //Sphere(Renderer::scene, glm::vec3(0.0, 1.5, 0.0), 0.2, 4);
    //Sphere(Renderer::scene, glm::vec3(5000.0, 5000.0, 5000.0), 500.0, 4);

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

    while (!glfwWindowShouldClose(Renderer::window))
    {
        if (Renderer::camera.moving)
        {
            currAccumPass = 0;

            glClear(GL_COLOR_BUFFER_BIT);

            Renderer::camera.moving = false;
        }

        Renderer::camera.ProcessInput(Renderer::window, deltaTime);
        Renderer::camera.UpdateView();
        
        // Normal debug view
        if (glfwGetKey(Renderer::window, GLFW_KEY_N))
        {
            if (!keyPressed)
            {
                debugNormal = !debugNormal;
                computeProgram.Use();
                computeProgram.SetUniform1i("debugNormal", debugNormal);
                computeProgram.Unuse();
            }
            keyPressed = true;
        }
        else
        {
            keyPressed = false;
        }
        
        computeProgram.Use();
        glBindFramebuffer(GL_FRAMEBUFFER, Renderer::rtFboID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Renderer::accumTexID);

        currAccumPass++;
        computeProgram.SetUniform1i("currAccumPass", currAccumPass);
        computeProgram.SetUniformCamera(Renderer::camera);
        
        glDispatchCompute(Renderer::screenWidth / 8, Renderer::screenHeight / 8, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        computeProgram.Unuse();

        computeAccumProgram.Use();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenQuadIndices);
        computeAccumProgram.Unuse();

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