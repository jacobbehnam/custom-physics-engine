#include "Scene.h"
#include <iostream>
#include <algorithm>

#include "graphics/utils/MathUtils.h"
#include <graphics/core/ResourceManager.h>
#include <glm/gtc/type_ptr.hpp>

Scene::Scene(OpenGLWindow* win, Physics::PhysicsSystem* physicsSys) : window(win), physicsSystem(physicsSys), currentGizmo(nullptr), camera(Camera(glm::vec3(0.0f, 0.0f, 3.0f))), basicShader(nullptr), cameraUBO(2*sizeof(glm::mat4), 0), hoverUBO(sizeof(glm::ivec4) * 1024, 1) {
    ResourceManager::loadPrimitives();
    basicShader = ResourceManager::loadShader("../shaders/primitive/primitive.vert", "../shaders/primitive/primitive.frag", "basic");
    SceneObject *cube = createPrimitive(Primitive::SPHERE, basicShader, true, glm::vec3(0.0f,1.0f,0.0f));
    cube->physicsBody->applyForce(glm::vec3(-1.0f, -1.0f, 0.0f));
    SceneObject *cube2 = createPrimitive(Primitive::SPHERE, basicShader, true, glm::vec3(-1.0f, 0.0f, 0.0f));

    cameraUBO.updateData(glm::value_ptr(camera.getProjMatrix()), sizeof(glm::mat4), 0);
}

SceneObject* Scene::createPrimitive(Primitive type, Shader *shader, bool wantPhysics, const glm::vec3& initPos) {
    SceneObject* primitive = nullptr;
    switch (type) {
        case Primitive::CUBE:
            primitive = new SceneObject(this, ResourceManager::getMesh("prim_cube"), shader, wantPhysics, initPos);
            break;
        case Primitive::SPHERE:
            primitive = new SceneObject(this, ResourceManager::getMesh("prim_sphere"), shader, wantPhysics, initPos);
            break;
    }
    assert(primitive != nullptr);

    return primitive;
}


uint32_t Scene::allocateObjectID() {
    if (!freeIDs.empty()) {
        uint32_t id = freeIDs.back();
        freeIDs.pop_back();
        return id;
    }
    return nextID++;
}

void Scene::freeObjectID(uint32_t objID) {
    freeIDs.push_back(objID);
}

void Scene::draw() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    cameraUBO.updateData(glm::value_ptr(camera.getViewMatrix()), sizeof(glm::mat4), sizeof(glm::mat4));

    float closestT;
    IPickable* hovered = findFistHit(pickableObjects, getMouseRay(), closestT, currentGizmo);

    if (hovered != nullptr) {
        hoveredIDs.insert(hovered->getObjectID());
    }

    std::vector<glm::ivec4> hoverVec(1024, glm::ivec4(0));
    for (uint32_t id : hoveredIDs) {
        hoverVec[id] = glm::ivec4(1);
    }

    hoverUBO.updateData(hoverVec.data(), hoverVec.size() * sizeof(glm::ivec4));

    // === INSTANCED DRAWING FOR CUBES ===
    std::vector<InstanceData> cubeInstances;
    Mesh* cubeMesh = ResourceManager::getMesh("prim_sphere");
    for (IDrawable* obj : drawableObjects) {
        if (obj->getMesh() == cubeMesh) {
            InstanceData instance;
            instance.model = obj->getModelMatrix();
            instance.objectID = obj->getObjectID();
            cubeInstances.push_back(instance);
        }
    }

    if (!cubeInstances.empty()) {
        basicShader->use();
        cubeMesh->drawInstanced(cubeInstances);
    }

    // === NORMAL DRAWING FOR OTHER OBJECTS ===
    for (IDrawable* obj : drawableObjects) {
        if (obj->getMesh() == cubeMesh)
            continue;

        obj->draw();
    }

    hoveredIDs.clear();
}

MathUtils::Ray Scene::getMouseRay() {
    QPointF mousePos = window->getMousePos();
    QSize fbSize = window->getFramebufferSize();

    return {
        camera.position,
        MathUtils::screenToWorldRayDirection(
            mousePos.x(), mousePos.y(),
            fbSize.width(), fbSize.height(),
            camera.getViewMatrix(), camera.getProjMatrix())
    };
}

IPickable *Scene::findFistHit(const std::vector<IPickable *> &objects, const MathUtils::Ray &ray, float &outT, IPickable *priority) {
    outT = std::numeric_limits<float>::infinity();
    IPickable* best = nullptr;

    for (IPickable* obj : objects) {
        float t;
        if (!obj->rayIntersection(ray.origin, ray.dir, t))
            continue;

        // If this is the priority object, take it immediately
        if (obj == priority) {
            outT = t;
            return obj;
        }

        if (t < outT) {
            outT = t;
            best = obj;
        }
    }

    return best;
}

void Scene::processInput(float dt) {
    // Mouse stuff
    QPointF mousePos = window->getMousePos();
    double mouseCurrX = mousePos.x();
    double mouseCurrY = mousePos.y();

    bool hasMoved = (std::abs(mouseCurrX - mouseLastX) > 1e-2 || std::abs(mouseCurrY - mouseLastY) > 1e-2);
    mouseDragging = (window->isMouseButtonHeld(Qt::LeftButton) || window->isMouseButtonHeld(Qt::RightButton)) && hasMoved;

    mouseLastX = mouseCurrX;
    mouseLastY = mouseCurrY;

    if (window->isKeyPressed(Qt::Key_Escape))
        qApp->quit();

    static const std::unordered_map<int, GizmoType> keyToGizmoType = {
        {Qt::Key_T, GizmoType::TRANSLATE},
        {Qt::Key_R, GizmoType::ROTATE},
        {Qt::Key_E, GizmoType::SCALE}
    };

    for (auto& [key, type] : keyToGizmoType) {
        if (window->isKeyPressed(key)) {
            GizmoType oldType = selectedGizmoType;
            selectedGizmoType = type;
            if (currentGizmo && oldType != selectedGizmoType)
                setGizmoFor(currentGizmo->getTarget(), true);
            break;
        }
    }

    if (currentGizmo) {
        currentGizmo->draw();
        if (currentGizmo->getIsDragging()) {
            hoveredIDs.insert(currentGizmo->getActiveHandle()->getObjectID());
            MathUtils::Ray ray = getMouseRay();
            currentGizmo->handleDrag(ray.origin, ray.dir);
        }
    }

    // if (window->isMouseCaptured()) {
    //     if (camera.firstMouse) {
    //         camera.handleMouseMovement(mouseLastXBeforeCapture, mouseLastYBeforeCapture);
    //     } else
    //     camera.handleMouseMovement(mouseLastX, mouseLastY);
    // }

    if (window->isKeyPressed(Qt::Key_A))
        camera.processKeyboard(Movement::LEFT, dt);
    if (window->isKeyPressed(Qt::Key_D))
        camera.processKeyboard(Movement::RIGHT, dt);
    if (window->isKeyPressed(Qt::Key_W))
        camera.processKeyboard(Movement::FORWARD, dt);
    if (window->isKeyPressed(Qt::Key_S))
        camera.processKeyboard(Movement::BACKWARD, dt);

    if (window->isKeyPressed(Qt::Key_Z)) {
        physicsSystem->enablePhysics();
    }
    if (window->isKeyPressed(Qt::Key_X)) {
        physicsSystem->disablePhysics();
    }
}

void Scene::handleMouseButton(int button, int action, int mods) {
    mouseLeftHeld = (button == Qt::LeftButton && action == QEvent::MouseButtonPress);
    mouseRightHeld = (button == Qt::RightButton && action == QEvent::MouseButtonPress);

    if (button == Qt::RightButton) {
        if (mouseRightHeld && !mouseCaptured) {
            mouseCaptured = true;
            QPointF mousePos = window->getMousePos();
            mouseLastXBeforeCapture = mousePos.x();
            mouseLastYBeforeCapture = mousePos.y();
            window->setMouseCaptured(true);
            camera.resetMouse();
        }
        else if (!mouseRightHeld && mouseCaptured) {
            mouseCaptured = false;
            window->setMouseCaptured(false);
            camera.resetMouse();
        }
    }
    else if (mouseLeftHeld) {
        // Get cursor as ray in world space
        MathUtils::Ray ray = getMouseRay();

        float closestDistance = std::numeric_limits<float>::max();
        IPickable* clickedObject = findFistHit(pickableObjects, ray, closestDistance, currentGizmo);

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

void Scene::setGizmoFor(SceneObject *newTarget, bool redraw) {
    if (currentGizmo) {
        if (currentGizmo->getTarget() == newTarget && redraw == false) {
            deleteGizmo();
        } else {
            deleteGizmo();
            currentGizmo = new Gizmo(selectedGizmoType, this, ResourceManager::getMesh("prim_cube"), newTarget);
        }
    } else {
        currentGizmo = new Gizmo(selectedGizmoType, this, ResourceManager::getMesh("prim_cube"), newTarget);
    }
}

void Scene::deleteGizmo() {
    drawableObjects.erase(std::remove(drawableObjects.begin(), drawableObjects.end(), currentGizmo), drawableObjects.end());
    pickableObjects.erase(std::remove(pickableObjects.begin(), pickableObjects.end(), currentGizmo), pickableObjects.end());
    delete currentGizmo;
    currentGizmo = nullptr;
}