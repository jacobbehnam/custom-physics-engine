#include "Scene.h"

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

Scene::Scene(GLFWwindow *win) : window(win), handle(nullptr), camera(Camera(glm::vec3(0.0f, 0.0f, 3.0f))), basicShader(Shader("../vertexShader.glsl", "../fragmentShader.glsl")){
    Mesh* cubeMesh = new Mesh(vertices, indices);
    SceneObject* cube = new SceneObject(this, cubeMesh, &basicShader);
    cube->setPosition(glm::vec3(0.0f,0.0f,0.0f));
    sceneObjects.push_back(cube);

    glfwSetWindowUserPointer(window, this);
}

void Scene::draw() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjMatrix();

    // TODO: Optimize later by only setting the view and projection matrix per shader rather than per object
    for (SceneObject* obj: sceneObjects) {
        obj->getShader()->use();
        obj->getShader()->setMat4("view", view);
        obj->getShader()->setMat4("projection", projection);
        obj->draw();
    }
}

void Scene::addObject(SceneObject* obj) {
    sceneObjects.push_back(obj);
}

void Scene::processInput(float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (mouseCaptured) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        camera.handleMouseMovement(xpos, ypos);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(Movement::LEFT, dt);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(Movement::RIGHT, dt);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(Movement::FORWARD, dt);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(Movement::BACKWARD, dt);
}

void Scene::handleMouseButton(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS && !mouseCaptured) {
            mouseCaptured = true;
            camera.resetMouse();
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if (action == GLFW_RELEASE && mouseCaptured) {
            mouseCaptured = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}




