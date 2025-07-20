#include "SceneManager.h"

#include <iostream>

#include "graphics/core/ResourceManager.h"
#include "ui/OpenGLWindow.h"

SceneManager::SceneManager(OpenGLWindow* win, Scene *scn) : window(win), scene(scn) {
    // TODO: preload shaders in resourcemanager (rn its in Scene)
}

void SceneManager::defaultSetup() {
    Shader* basicShader = ResourceManager::getShader("basic");
    SceneObject *cube = createPrimitive(Primitive::SPHERE, basicShader, true, glm::vec3(0.0f,1.0f,0.0f));
    cube->physicsBody->applyForce(glm::vec3(-1.0f, -1.0f, 0.0f));
    SceneObject *cube2 = createPrimitive(Primitive::SPHERE, basicShader, true, glm::vec3(-1.0f, 0.0f, 0.0f));
}

SceneObject* SceneManager::createPrimitive(Primitive type, Shader *shader, bool wantPhysics, const glm::vec3 &initPos) {
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
    sceneObjects.push_back(primitive);
    addDrawable(primitive);
    addPickable(primitive);
    emit objectAdded(primitive);

    return primitive;
}

void SceneManager::addToPhysicsSystem(IPhysicsBody *body) const {
    if (scene->physicsSystem)
        scene->physicsSystem->addBody(body);
}

void SceneManager::deleteObject(SceneObject *obj) {
    if (!obj) return;

    sceneObjects.erase(
        std::remove(sceneObjects.begin(), sceneObjects.end(), obj),
        sceneObjects.end());

    scene->deleteSceneObject(obj);
    emit objectRemoved(obj);
}

MathUtils::Ray SceneManager::getMouseRay() {
    QPointF mousePos = window->getMousePos();
    QSize fbSize = window->getFramebufferSize();

    return {
        scene->getCamera()->position,
        MathUtils::screenToWorldRayDirection(
            mousePos.x(), mousePos.y(),
            fbSize.width(), fbSize.height(),
            scene->getCamera()->getViewMatrix(), scene->getCamera()->getProjMatrix())
    };
}

void SceneManager::updateHoverState(const MathUtils::Ray &mouseRay) {
    hoveredIDs.clear();

    float closestT;
    IPickable* hovered = MathUtils::findFirstHit(pickableObjects, mouseRay, closestT, currentGizmo.get());
    if (hovered) {
        hoveredIDs.insert(hovered->getObjectID());
    }
}

void SceneManager::handleMouseButton(Qt::MouseButton button, QEvent::Type type, Qt::KeyboardModifiers mods) {
    const bool isPress = (type == QEvent::MouseButtonPress);
    const bool isRelease = (type == QEvent::MouseButtonRelease);
    Camera* camera = scene->getCamera();

    if (button == Qt::RightButton) {
        if (isPress && !window->isMouseCaptured()) {
            window->setMouseCaptured(true);
            camera->resetMouse();
        } else if (isRelease && window->isMouseCaptured()) {
            window->setMouseCaptured(false);
            camera->resetMouse();
        }
    }

    if (button == Qt::LeftButton) {
        MathUtils::Ray ray = getMouseRay();
        float closestDistance = std::numeric_limits<float>::max();
        IPickable* clickedObject = MathUtils::findFirstHit(pickableObjects, ray, closestDistance, currentGizmo.get());
        const bool clickedCurrentGizmo = clickedObject && currentGizmo && (clickedObject->getObjectID() == currentGizmo->getObjectID());

        if (isPress) {
            if (!(clickedObject && (selectedIDs.empty() || clickedCurrentGizmo))) {
                // If we select a non gizmo not currently selected, clear the selection list and delete the gizmo
                selectedIDs.clear();
                if (currentGizmo)
                    deleteCurrentGizmo();
            }
            if (clickedObject) {
                clickedObject->handleClick(ray.origin, ray.dir, closestDistance);
                selectedIDs.insert(clickedObject->getObjectID());
                if (!clickedCurrentGizmo)
                    // Fine with dynamic cast here because this method is not executed often
                    emit selectedItem(dynamic_cast<SceneObject*>(clickedObject));
            } else {
                emit selectedItem(nullptr); // Deselect all items in hierarchy
            }
        } else if (isRelease && currentGizmo) {
            currentGizmo->handleRelease();
            selectedIDs.erase(currentGizmo->getObjectID());
        }
    }
}

void SceneManager::processHeldKeys(const QSet<int> &heldKeys, float dt) {
    Camera* camera = scene->getCamera();

    if (heldKeys.contains(Qt::Key_Escape))
        qApp->quit();

    static const std::unordered_map<int, GizmoType> keyToGizmoType = {
        {Qt::Key_T, GizmoType::TRANSLATE},
        {Qt::Key_R, GizmoType::ROTATE},
        {Qt::Key_E, GizmoType::SCALE}
    };

    for (auto& [key, type] : keyToGizmoType) {
        if (heldKeys.contains(key)) {
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

    if (heldKeys.contains(Qt::Key_A))
        camera->processKeyboard(Movement::LEFT, dt);
    if (heldKeys.contains(Qt::Key_D))
        camera->processKeyboard(Movement::RIGHT, dt);
    if (heldKeys.contains(Qt::Key_W))
        camera->processKeyboard(Movement::FORWARD, dt);
    if (heldKeys.contains(Qt::Key_S))
        camera->processKeyboard(Movement::BACKWARD, dt);

    if (heldKeys.contains(Qt::Key_Z)) {
        scene->physicsSystem->enablePhysics();
    }
    if (heldKeys.contains(Qt::Key_X)) {
        scene->physicsSystem->disablePhysics();
    }
}

void SceneManager::setGizmoFor(SceneObject *newTarget, bool redraw) {
    if (currentGizmo) {
        if (currentGizmo->getTarget() == newTarget && redraw == false) {
            deleteCurrentGizmo();
        } else {
            deleteCurrentGizmo();
            currentGizmo = std::make_unique<Gizmo>(selectedGizmoType, this, ResourceManager::getMesh("prim_cube"), newTarget);
        }
    } else {
        currentGizmo = std::make_unique<Gizmo>(selectedGizmoType, this, ResourceManager::getMesh("prim_cube"), newTarget);
    }
}

void SceneManager::setSelectFor(SceneObject *obj, bool flag) {
    assert(obj);
    uint32_t objID = obj->getObjectID();
    if (flag) {
        if (selectedIDs.find(objID) == hoveredIDs.end())
            selectedIDs.insert(objID);
    } else {
        selectedIDs.erase(objID);
    }
}

void SceneManager::deleteCurrentGizmo() {
    removeDrawable(currentGizmo.get());
    removePickable(currentGizmo.get());
    currentGizmo.reset();
}

void SceneManager::removePickable(IPickable *obj) {
    pickableObjects.erase(
        std::remove(pickableObjects.begin(), pickableObjects.end(), obj),
        pickableObjects.end());
}