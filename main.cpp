#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/Mesh.h>
#include <graphics/SceneObject.h>
#include <graphics/Camera.h>
#include <graphics/Shader.h>
#include "graphics/TranslateHandle.h"
#include <graphics/Scene.h>

static Mesh*          gMesh         = nullptr;
static SceneObject*   gCube         = nullptr;
static Shader*   gShader = nullptr;
static TranslateHandle* gHandle = nullptr;

void framebuffer_size_callback (GLFWwindow* window, int width, int height) {
    glViewport(0,0,width,height);
}

bool rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir,
    glm::vec3 sphereCenter, float sphereRadius, float &outDistance) {
    // Assumes rayDir is normalized!
    // Math is solving for t when || P(t) - C || = r where P(t) = O + tD

    glm::vec3 OC = rayOrigin - sphereCenter;
    float b = 2.0f * glm::dot(OC, rayDir);
    float c = glm::dot(OC, OC) - sphereRadius * sphereRadius;
    float discriminant = b*b - 4*c;
    if (discriminant < 0.0f) return false;

    float sqrtDisc = glm::sqrt(discriminant);
    float t1 = (-b - sqrtDisc)/2.0f; // Entry ray
    float t2 = (-b + sqrtDisc)/2.0f; // Exit ray

    float t = (t1 >= 0.0f) ? t1 : t2; // If the ray began inside the object, only the exit value will be positive
    if (t < 0.0f) return false; // If both values are negative, then the object is behind the camera, so no intersection

    outDistance = t;
    return true;
}



void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    //     if (action == GLFW_PRESS && !mouseCaptured) {
    //         mouseCaptured = true;
    //         firstMouse = true;
    //         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //     }
    //     else if (action == GLFW_RELEASE && mouseCaptured) {
    //         mouseCaptured = false;
    //         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    //     }
    // }
    // else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    //     // Get cursor as ray in world space
    //     double mouseX, mouseY;
    //     glfwGetCursorPos(window, &mouseX, &mouseY);
    //
    //     float x = (2.0f * mouseX) / 800 - 1.0f;
    //     float y = 1.0f - (2.0f * mouseY) / 600;
    //
    //     glm::vec4 clip = glm::vec4(x, y, -1.0f, 1.0f);
    //     Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    //     glm::mat4 invProj = glm::inverse(cam->getProjMatrix());
    //     glm::vec4 eye = invProj * clip;
    //     eye.z = -1.0f; eye.w = 0.0f;
    //     glm::mat4 invView = glm::inverse(cam->getViewMatrix());
    //     glm::vec4 worldDir4 = invView * eye;
    //     glm::vec3 worldDir = glm::normalize(glm::vec3(worldDir4));
    //
    //     float closestDistance = std::numeric_limits<float>::max();
    //     SceneObject* clickedObject = nullptr;
    //
    //     for (auto obj : sceneObjects) {
    //         float t;
    //         if (rayIntersection(cam->position, worldDir, obj->getPosition(), obj->getBoundingRadius(), t)) {
    //             if (t < closestDistance) {
    //                 closestDistance = t;
    //                 clickedObject = obj;
    //             }
    //         }
    //     }
    //
    //     if (clickedObject) {
    //         std::cout << "Clicked " << clickedObject << " at t=" << closestDistance << "\n";
    //         delete gHandle;
    //         gHandle = new TranslateHandle(gMesh, gShader, gCube, Axis::X);
    //     }
    // }
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

    Scene scene(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int b, int a, int m){
        Scene* scene = static_cast<Scene*>(glfwGetWindowUserPointer(w));
        if (scene)
            scene->handleMouseButton(b, a, m);
    });

    // === 7. Main render loop ===
    while (!glfwWindowShouldClose(window)) {
        double currentFrame = glfwGetTime();
        double deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        scene.processInput(deltaTime);
        scene.draw();

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}