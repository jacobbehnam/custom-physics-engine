#include "Scene.h"
#include <iostream>
#include <algorithm>

#include "graphics/utils/MathUtils.h"
#include <graphics/core/ResourceManager.h>
#include <glm/gtc/type_ptr.hpp>

std::vector<Vertex> vertices = {
    { {-0.5f, -0.5f, -0.5f}, {0.0f,0.0f,0.0f}, 1 },
    { {0.5f, -0.5f, -0.5f}, {0.0f,0.0f,0.0f}, 1 },
    { {0.5f, 0.5f, -0.5f}, {0.0f,0.0f,0.0f}, 1 },
    { {-0.5f, 0.5f, -0.5f}, {0.0f,0.0f,0.0f}, 1 },
    { {-0.5f, -0.5f, 0.5f}, {0.0f,0.0f,0.0f}, 1 },
    { {0.5f, -0.5f, 0.5f}, {0.0f,0.0f,0.0f}, 1 },
    { {0.5f, 0.5f, 0.5f}, {0.0f,0.0f,0.0f}, 1 },
    { {-0.5f, 0.5f, 0.5f}, {0.0f,0.0f,0.0f}, 1 }
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

Scene::Scene(GLFWwindow *win) : window(win), currentGizmo(nullptr), camera(Camera(glm::vec3(0.0f, 0.0f, 3.0f))), basicShader(nullptr), cameraUBO(2*sizeof(glm::mat4), 0) {
    basicShader = ResourceManager::loadShader("../vertexShader.glsl", "../fragmentShader.glsl", "basic");
    Mesh* cubeMesh = ResourceManager::loadMesh(vertices, indices, "cube");
    SceneObject *cube = new SceneObject(this, cubeMesh, basicShader);
    cube->setPosition(glm::vec3(1.0f, 0.0f, 0.0f));
    SceneObject *cube2 = new SceneObject(this, cubeMesh, basicShader);
    cube2->setPosition(glm::vec3(-1.0f, 0.0f, 0.0f));

    cameraUBO.updateData(glm::value_ptr(camera.getProjMatrix()), sizeof(glm::mat4), 0);

    glfwSetWindowUserPointer(window, this);
}

void Scene::draw() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    cameraUBO.updateData(glm::value_ptr(camera.getViewMatrix()), sizeof(glm::mat4), sizeof(glm::mat4));

    // TODO: Optimize later by only setting the view and projection matrix per shader rather than per object. Also repeated calls of shader.use() is not good
    MathUtils::Ray ray = getMouseRay();
    float closestT = std::numeric_limits<float>::infinity();
    IPickable* hovered = nullptr;

    for (IPickable* obj : pickableObjects) {
        float t;
        if (obj->rayIntersection(ray.origin, ray.dir, t)) {
            if (t < closestT) {
                closestT = t;
                hovered = obj;
            }
        }
    }

    // Mark only the hovered one as hovered
    for (IPickable* obj : pickableObjects) {
        obj->setHovered(obj == hovered);
    }

    for (IDrawable* obj: drawableObjects) {
        obj->getShader()->use();

        bool hovered = false;
        if (IPickable* pickable = dynamic_cast<IPickable*>(obj)) {
            hovered = pickable->getHovered();
        }

        obj->getShader()->setBool("isHovered", hovered);
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

    if (currentGizmo) {
        currentGizmo->draw();
        if (currentGizmo->isDragging) {
            MathUtils::Ray ray = getMouseRay();
            currentGizmo->handleDrag(ray.origin, ray.dir);
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
        if (currentGizmo)
            currentGizmo->handleRelease();
    }
}

Camera *Scene::getCamera() {
    return &camera;
}

void Scene::setGizmoFor(SceneObject *newTarget) {
    if (currentGizmo) {
        if (currentGizmo->getTarget() == newTarget) {
            deleteGizmo();
        } else {
            deleteGizmo();
            currentGizmo = new Gizmo(GizmoType::SCALE, this, ResourceManager::getMesh("cube"), newTarget, ResourceManager::getShader("basic"));
        }
    } else {
        currentGizmo = new Gizmo(GizmoType::SCALE, this, ResourceManager::getMesh("cube"), newTarget, ResourceManager::getShader("basic"));
    }
}

void Scene::deleteGizmo() {
    drawableObjects.erase(std::remove(drawableObjects.begin(), drawableObjects.end(), currentGizmo), drawableObjects.end());
    pickableObjects.erase(std::remove(pickableObjects.begin(), pickableObjects.end(), currentGizmo), pickableObjects.end());
    delete currentGizmo;
    currentGizmo = nullptr;
}


