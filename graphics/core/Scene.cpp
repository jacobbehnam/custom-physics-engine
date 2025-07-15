#include "Scene.h"
#include <iostream>
#include <algorithm>

#include "graphics/utils/MathUtils.h"
#include <graphics/core/ResourceManager.h>
#include <glm/gtc/type_ptr.hpp>

Scene::Scene(OpenGLWindow* win) : window(win), physicsSystem(std::make_unique<Physics::PhysicsSystem>()), currentGizmo(nullptr), camera(Camera(glm::vec3(0.0f, 0.0f, 3.0f))), basicShader(nullptr), cameraUBO(2*sizeof(glm::mat4), 0), hoverUBO(sizeof(glm::ivec4) * 1024, 1) {
    ResourceManager::loadPrimitives();
    basicShader = ResourceManager::loadShader("../shaders/primitive/primitive.vert", "../shaders/primitive/primitive.frag", "basic");
    cameraUBO.updateData(glm::value_ptr(camera.getProjMatrix()), sizeof(glm::mat4), 0);
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
    IPickable* hovered = findFirstHit(pickableObjects, getMouseRay(), closestT, currentGizmo);

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

void Scene::update(float dt) {
    physicsSystem->step(dt);
    processInput(dt);
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

IPickable *Scene::findFirstHit(const std::vector<IPickable *> &objects, const MathUtils::Ray &ray, float &outT, IPickable *priority) {
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
    // TODO: make a separate input handler class?
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

    if (currentGizmo && currentGizmo->getIsDragging()) {
        hoveredIDs.insert(currentGizmo->getActiveHandle()->getObjectID());
        MathUtils::Ray ray = getMouseRay();
        currentGizmo->handleDrag(ray.origin, ray.dir);
    }

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

void Scene::handleMouseButton(Qt::MouseButton button, QEvent::Type eventType, Qt::KeyboardModifiers mods) {
    const bool isPress = (eventType == QEvent::MouseButtonPress);
    const bool isRelease = (eventType == QEvent::MouseButtonRelease);

    if (button == Qt::RightButton) {
        if (isPress && !window->isMouseCaptured()) {
            window->setMouseCaptured(true);
            camera.resetMouse();
        } else if (isRelease && window->isMouseCaptured()) {
            window->setMouseCaptured(false);
            camera.resetMouse();
        }
    }

    if (button == Qt::LeftButton) {
        if (isPress) {
            MathUtils::Ray ray = getMouseRay();
            float closestDistance = std::numeric_limits<float>::max();
            IPickable* clickedObject = findFirstHit(pickableObjects, ray, closestDistance, currentGizmo);
            if (clickedObject) {
                clickedObject->handleClick(ray.origin, ray.dir, closestDistance);
            }
        } else if (isRelease && currentGizmo) {
            currentGizmo->handleRelease();
        }
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

void Scene::deleteSceneObject(SceneObject *obj) {
    if (!obj) return;

    drawableObjects.erase(
        std::remove(drawableObjects.begin(), drawableObjects.end(), static_cast<IDrawable*>(obj)),
        drawableObjects.end()
    );

    pickableObjects.erase(
        std::remove(pickableObjects.begin(), pickableObjects.end(), static_cast<IPickable*>(obj)),
        pickableObjects.end()
    );

    if (obj->physicsBody) {
        physicsSystem->removeBody(obj->physicsBody);
    }

    delete obj;
}
