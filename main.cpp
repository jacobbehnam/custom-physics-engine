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


void processInput(GLFWwindow* window, Camera& camera, float deltaTime, SceneObject &cube) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(Movement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(Movement::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(Movement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(Movement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        cube.setRotation(glm::vec3(1.0f, 0.0f, 0.0f));
}

// TODO: Change these from global variables later
static bool firstMouse = true;
static bool mouseCaptured = false;
static float lastX = 0.0f;
static float lastY = 0.0f;


void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!mouseCaptured) return;
    // Retrieve camera instance
    Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (!cam) return;

    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    // Calculate movement offset
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos; // reversed since y-coordinates go from bottom to top

    lastX = (float)xpos;
    lastY = (float)ypos;

    // Pass offsets to camera
    cam->processMouseMovement(xoffset, yoffset);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS && !mouseCaptured) {
            mouseCaptured = true;
            firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if (action == GLFW_RELEASE && mouseCaptured) {
            mouseCaptured = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Get cursor as ray in world space
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        float x = (2.0f * mouseX) / 800 - 1.0f;
        float y = 1.0f - (2.0f * mouseY) / 600;

        glm::vec4 clip = glm::vec4(x, y, -1.0f, 1.0f);
        Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
        glm::mat4 invProj = glm::inverse(cam->getProjMatrix());
        glm::vec4 eye = invProj * clip;
        eye.z = -1.0f; eye.w = 0.0f;
        glm::mat4 invView = glm::inverse(cam->getViewMatrix());
        glm::vec4 worldDir4 = invView * eye;
        glm::vec3 worldDir = glm::normalize(glm::vec3(worldDir4));

        float closestDistance = std::numeric_limits<float>::max();
        SceneObject* clickedObject = nullptr;

        for (auto obj : sceneObjects) {
            float t;
            if (rayIntersection(cam->position, worldDir, obj->getPosition(), obj->getBoundingRadius(), t)) {
                if (t < closestDistance) {
                    closestDistance = t;
                    clickedObject = obj;
                }
            }
        }

        if (clickedObject) {
            std::cout << "Clicked " << clickedObject << " at t=" << closestDistance << "\n";
            delete gHandle;
            gHandle = new TranslateHandle(gMesh, gShader, gCube, Axis::X);
        }
    }
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

    // === 1. Vertex data ===
    std::vector<Vertex> vertices = {
        { {-0.5f, -0.5f, -0.5f}, {0.0f,0.0f,0.0f} },
        { {0.5f, -0.5f, -0.5f}, {0.0f,0.0f,0.0f} },
        { {0.5f, 0.5f, -0.5f}, {0.0f,0.0f,0.0f} },
        { {-0.5f, 0.5f, -0.5f}, {0.0f,0.0f,0.0f} },
        { {-0.5f, -0.5f, 0.5f}, {0.0f,0.0f,0.0f} },
        { {0.5f, -0.5f, 0.5f}, {0.0f,0.0f,0.0f} },
        { {0.5f, 0.5f, 0.5f}, {0.0f,0.0f,0.0f} },
        { {-0.5f, 0.5f, 0.5f}, {0.0f,0.0f,0.0f} }
    };

    std::vector<unsigned int> indices {
        0, 1, 2,
        0, 3, 2,
        0, 1, 5,
        0, 4, 5,
        0, 4, 7,
        0, 3, 7,
        1, 2, 6,
        1, 5, 6,
        2, 3, 7,
        2, 6, 7,
        4, 5, 6,
        4, 7, 6
    };

    glEnable(GL_DEPTH_TEST);
    glLineWidth(10.0f);

    Shader cubeShader("../vertexShader.glsl", "../fragmentShader.glsl");

    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    glm::mat4 view;
    glm::mat4 projection = camera.getProjMatrix();
    Camera* camPtr = &camera;
    glfwSetWindowUserPointer(window, camPtr);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mouse_callback);

    double lastFrame = glfwGetTime();

    Mesh mesh(vertices, indices);
    SceneObject cube(&mesh, &cubeShader);

    gShader = &cubeShader;
    gMesh  = &mesh;
    gCube  = &cube;

    // === 7. Main render loop ===
    while (!glfwWindowShouldClose(window)) {
        double currentFrame = glfwGetTime();
        double deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, camera, deltaTime, cube);
        view = camera.getViewMatrix();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO: Create a set of shaders to iterate over
        cubeShader.setMat4("view", view);
        cubeShader.setMat4("projection", projection);

        cube.draw();
        if (gHandle) {
            gHandle->draw();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}