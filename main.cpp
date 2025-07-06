#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <graphics/components/Mesh.h>
#include <graphics/core/SceneObject.h>
#include <graphics/components/Shader.h>
#include "graphics/components/TranslateHandle.h"
#include <graphics/core/Scene.h>

#include "physics/PhysicsSystem.h"

void framebuffer_size_callback (GLFWwindow* window, int width, int height) {
    glViewport(0,0,width,height);
}

int main() {
    glfwInit();

    // We are on OpenGL version 4.6
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Test", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0,0,800,600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glEnable(GL_DEPTH_TEST);
    glLineWidth(10.0f);

    double lastFrame = glfwGetTime();

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int b, int a, int m){
        Scene* scene = static_cast<Scene*>(glfwGetWindowUserPointer(w));
        if (scene)
            scene->handleMouseButton(b, a, m);
    });

    // Without this, the camera is pretty choppy
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    } else {
        std::cout << "Raw mouse movement not supported" << std::endl;
    }

    Physics::PhysicsSystem physicsSystem;
    Scene scene(window, &physicsSystem);

    // === 7. Main render loop ===
    while (!glfwWindowShouldClose(window)) {
        double currentFrame = glfwGetTime();
        double deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        physicsSystem.step(deltaTime);

        glfwPollEvents();
        scene.processInput(deltaTime);
        scene.draw();

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}