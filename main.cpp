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


void framebuffer_size_callback (GLFWwindow* window, int width, int height) {
    glViewport(0,0,width,height);
}

void processInput(GLFWwindow* window, Camera& camera, float deltaTime, SceneObject &cube) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(Camera::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(Camera::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(Camera::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(Camera::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        cube.setRotation(glm::vec3(1.0f, 0.0f, 0.0f));
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;
    static float lastX = 0.0f;
    static float lastY = 0.0f;

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
        if (action == GLFW_PRESS) {

            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if (action == GLFW_RELEASE) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

std::string loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: Could not open file " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    std::string vertexSource = loadFile("../vertexShader.glsl");
    std::string fragmentSource = loadFile("../fragmentShader.glsl");
    std::string edgeSource = loadFile("../edgeShader.glsl");
    const char* vertexShaderSource = vertexSource.c_str();
    const char* fragmentShaderSource = fragmentSource.c_str();
    const char* edgeShaderSource = edgeSource.c_str();

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

    unsigned int edges[] {
        0,1, 1,2, 2,3, 3,0, // Bottom
        4,5, 5,6, 6,7, 7,4, // Top
        0,4, 1,5, 2,6, 3,7 // Vertical
    };

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    unsigned int edgeShader;
    edgeShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(edgeShader, 1, &edgeShaderSource, NULL);
    glCompileShader(edgeShader);
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    unsigned int edgeShaderProgram = glCreateProgram();
    glAttachShader(edgeShaderProgram, edgeShader);
    glAttachShader(edgeShaderProgram, fragmentShader);
    glLinkProgram(edgeShaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(edgeShader);

    // int tmatUniform = glGetUniformLocation(shaderProgram, "tmat");
    // int tmat2Uniform = glGetUniformLocation(edgeShaderProgram, "tmat");
    int camUniform = glGetUniformLocation(shaderProgram, "view");
    int projUniform = glGetUniformLocation(shaderProgram, "projection");

    glEnable(GL_DEPTH_TEST);
    glLineWidth(10.0f);

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
    SceneObject cube(&mesh, shaderProgram);

    // === 7. Main render loop ===
    while (!glfwWindowShouldClose(window)) {
        double currentFrame = glfwGetTime();
        double deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, camera, deltaTime, cube);
        view = camera.getViewMatrix();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(camUniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projUniform, 1, GL_FALSE, glm::value_ptr(projection));
        cube.draw();

        // glBindVertexArray(VAO);
        // glEnable(GL_POLYGON_OFFSET_FILL);     // enables offset for fill mode
        // glPolygonOffset(4.0f, 100.0f);          // adjust the offset
        // glUseProgram(shaderProgram);          // your fill shader
        // glUniformMatrix4fv(tmatUniform, 1, GL_FALSE, glm::value_ptr(tmat));
        // glUniformMatrix4fv(camUniform, 1, GL_FALSE, glm::value_ptr(view));
        // glUniformMatrix4fv(projUniform, 1, GL_FALSE, glm::value_ptr(proj));
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // triangles
        // glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        // glDisable(GL_POLYGON_OFFSET_FILL);    // stop offset after drawing the filled cube

        // glUseProgram(edgeShaderProgram);
        // glUniformMatrix4fv(tmat2Uniform, 1, GL_FALSE, glm::value_ptr(tmat));
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeEBO);
        // glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}