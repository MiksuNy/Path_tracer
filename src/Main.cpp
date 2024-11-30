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



    Mesh mesh("res/meshes/material_test/sword_and_ground", Renderer::scene.materials);



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