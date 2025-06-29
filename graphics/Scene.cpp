#include "Scene.h"
#include <iostream>
#include <algorithm>

#include "MathUtils.h"

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

Scene::Scene(GLFWwindow *win) : window(win), translationGizmo(nullptr), camera(Camera(glm::vec3(0.0f, 0.0f, 3.0f))), basicShader(Shader("../vertexShader.glsl", "../fragmentShader.glsl")){
    Mesh* cubeMesh = new Mesh(vertices, indices);
    SceneObject* cube = new SceneObject(this, cubeMesh, &basicShader);
    cube->setPosition(glm::vec3(1.0f,0.0f,0.0f));
    SceneObject* cube2 = new SceneObject(this, cubeMesh, &basicShader);
    cube2->setPosition(glm::vec3(-1.0f,0.0f,0.0f));

    glfwSetWindowUserPointer(window, this);
}

void Scene::draw() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjMatrix();

    // TODO: Optimize later by only setting the view and projection matrix per shader rather than per object
    for (IDrawable* obj: drawableObjects) {
        obj->getShader()->use();
        obj->getShader()->setMat4("view", view);
        obj->getShader()->setMat4("projection", projection);
        obj->draw();
    }
}

MathUtils::Ray Scene::getMouseRay() {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    Scene* scene = static_cast<Scene*>(glfwGetWindowUserPointer(window));
    Camera* cam = (scene->getCamera());
    return {cam->position, MathUtils::screenToWorldRayDirection(mouseX, mouseY, fbWidth, fbHeight, cam->getViewMatrix(), cam->getProjMatrix())};
}

void Scene::addObject(IDrawable* obj) {
    drawableObjects.push_back(obj);
}

void Scene::addObject(IPickable* obj) {
    pickableObjects.push_back(obj);
}

void Scene::processInput(float dt) {
    // Mouse stuff
    double mouseCurrX, mouseCurrY;
    glfwGetCursorPos(window, &mouseCurrX, &mouseCurrY);
    bool hasMoved = (std::abs(mouseCurrX - mouseLastX) > 1e-2 || std::abs(mouseCurrY - mouseLastY) > 1e-2);
    mouseDragging = (mouseLeftHeld || mouseRightHeld) && hasMoved;
    mouseLastX = mouseCurrX;
    mouseLastY = mouseCurrY;

    if (translationGizmo) {
        translationGizmo->draw();
        if (translationGizmo->isDragging) {
            MathUtils::Ray ray = getMouseRay();
            translationGizmo->handleDrag(ray.origin, ray.dir);
        }
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (mouseCaptured) {
        if (camera.firstMouse) {
            camera.handleMouseMovement(mouseLastXBeforeCapture, mouseLastYBeforeCapture);
        } else
        camera.handleMouseMovement(mouseLastX, mouseLastY);
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
    mouseLeftHeld = button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS;
    mouseRightHeld = button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (mouseRightHeld && !mouseCaptured) {
            mouseCaptured = true;
            glfwGetCursorPos(window, &mouseLastXBeforeCapture, &mouseLastYBeforeCapture);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            camera.resetMouse();
        }
        else if (!mouseRightHeld && mouseCaptured) {
            mouseCaptured = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetCursorPos(window, mouseLastXBeforeCapture, mouseLastYBeforeCapture);
            camera.resetMouse();
        }
    }
    else if (mouseLeftHeld) {
        // Get cursor as ray in world space
        MathUtils::Ray ray = getMouseRay();

        float closestDistance = std::numeric_limits<float>::max();
        IPickable* clickedObject = nullptr;

        for (auto obj : pickableObjects) {
            float t;
            if (obj->rayIntersection(ray.origin, ray.dir, t)) {
                if (t < closestDistance) {
                    closestDistance = t;
                    clickedObject = obj;
                }
            }
        }
        if (clickedObject) {
            mouseDragging = false;
            clickedObject->handleClick(ray.origin, ray.dir, closestDistance);
        }
    } else if (!mouseLeftHeld) {
        if (translationGizmo)
            translationGizmo->handleRelease();
    }
}

Camera *Scene::getCamera() {
    return &camera;
}

void Scene::setGizmoFor(SceneObject *newTarget) {
    if (translationGizmo) {
        if (translationGizmo->getTarget() == newTarget) {
            deleteGizmo();
        } else {
            deleteGizmo();
            translationGizmo = new Gizmo(this, newTarget->mesh, newTarget, newTarget->shader);
        }
    } else {
        translationGizmo = new Gizmo(this, newTarget->mesh, newTarget, newTarget->shader);
    }
}

void Scene::deleteGizmo() {
    drawableObjects.erase(std::remove(drawableObjects.begin(), drawableObjects.end(), translationGizmo), drawableObjects.end());
    pickableObjects.erase(std::remove(pickableObjects.begin(), pickableObjects.end(), translationGizmo), pickableObjects.end());
    delete translationGizmo;
    translationGizmo = nullptr;
}


