#include "graphics/core/SceneManager.h"

#include <iostream>
#include <QApplication>

#include "SceneSerializer.h"
#include "graphics/core/ResourceManager.h"
#include "physics/bounding/BoxCollider.h"
#include "graphics/core/SceneObject.h"
#include "ui/OpenGLWindow.h"
#include "graphics/debug/PathTraces.h"
#include "graphics/debug/Forces.h"
#include "graphics/debug/Colliders.h"
#include "graphics/presets/ScenePresets.h"
#include "physics/Constants.h"
#include "ui/AppSettings.h"
#include "ui/settings/DebugSettings.h"
#include "ui/settings/GraphicsSettings.h"
#include "graphics/raytrace/SceneRayTracer.h"

#include <algorithm>

SceneManager::SceneManager(OpenGLWindow* win, Scene *scn) : window(win), scene(scn), physicsSystem(std::make_unique<Physics::PhysicsSystem>()) {
    // TODO: preload shaders in resourcemanager (rn its in Scene)
    physicsSystem->start();
    initDebugDrawables();
}

SceneManager::~SceneManager() {
    const bool madeCurrent = window && window->context();
    if (madeCurrent) {
        window->makeCurrent();
    }
    removeDebugDrawables();
    if (madeCurrent) {
        window->doneCurrent();
    }
}

void SceneManager::defaultSetup() {
    resetScene();

    SceneObject* ball = createObject("prim_sphere", ResourceManager::getShader("basic"), PointMassOptions());
    setObjectName(ball, "Ball");
    ball->getPhysicsBody()->setVelocity(glm::vec3(0.0f, 15.0f, 0.0f), BodyLock::LOCK);

    ObjectOptions floorOpts;
    floorOpts.position = glm::vec3(0.0f, -0.5f, 0.0f);
    floorOpts.scale = glm::vec3(2000.0f, 1.0f, 300000.0f);

    SceneObject* floor = createObject("prim_cube", ResourceManager::getShader("checkerboard"), RigidBodyOptions::Box(floorOpts, true));
    removePickable(floor);
    setObjectName(floor, "Ground");

    PointMassOptions keysOptions{};
    keysOptions.base.position = glm::vec3(0.0f, 20.0f, 0.0f);
    SceneObject* keys = createObject("prim_sphere", ResourceManager::getShader("basic"), keysOptions);
    setObjectName(keys, "Keys");
}

void SceneManager::resetScene() {
    stopSimulation();
    physicsSystem->clearRuntimeState();
    deleteAllObjects();
    hoveredIDs.clear();
    selectedIDs.clear();
    stopCondition = {};
    SceneObject::setPhysicsPosMap(SceneObject::PosMap{});
    SceneObject::setRenderOrigin(glm::vec3(0.0f));

    if (window) {
        window->resetRenderClock();
    }

    if (scene && scene->getCamera()) {
        scene->getCamera()->resetView();
    }

    setGlobalAcceleration(glm::vec3(0.0f, -Constants::STANDARD_GRAVITY, 0.0f));
    setSimSpeed(1.0f);
    physicsSystem->setGravitationalConstant(Constants::G);
    physicsSystem->setAmbientTemperature(293.15f);
    setSelectFor(nullptr);
}

bool SceneManager::loadPreset(const ScenePresets::PresetDescriptor& preset) {
    if (!preset.generate) return false;

    resetScene();
    preset.generate(*this);
    applyDebugSettings();
    return true;
}

SceneObject* SceneManager::createPrimitive(Primitive type, Shader *shader = ResourceManager::getShader("basic"), const CreationOptions& options) {
    std::unique_ptr<SceneObject> primitive = nullptr;
    switch (type) {
        case Primitive::CUBE:
            primitive = std::make_unique<SceneObject>(this, "prim_cube", shader, options);
            break;
        case Primitive::SPHERE:
            primitive = std::make_unique<SceneObject>(this, "prim_sphere", shader, options);
            break;
    }
    assert(primitive != nullptr);
    SceneObject* ptr = primitive.get();

    sceneObjectsByID[ptr->getObjectID()] = ptr;
    sceneObjects.push_back(std::move(primitive));

    addDrawable(ptr);
    addPickable(ptr);
    emit objectAdded(ptr);

    return ptr;
}

SceneObject* SceneManager::createObject(const std::string &meshName, Shader *shader, const CreationOptions& options) {
    std::unique_ptr<SceneObject> primitive = std::make_unique<SceneObject>(this, meshName, shader, options);
    assert(primitive != nullptr);
    SceneObject* ptr = primitive.get();

    setObjectName(primitive.get(), makeUniqueName(generateDefaultName(options)));
    sceneObjectsByID[ptr->getObjectID()] = ptr;
    sceneObjects.push_back(std::move(primitive));

    addDrawable(ptr);
    addPickable(ptr);
    emit objectAdded(ptr);

    return ptr;
}

void SceneManager::deleteObject(SceneObject *obj) {
    if (!obj) return;

    if (getCameraTarget() == obj) {
        clearCameraTarget();
    }
    if (currentGizmo && currentGizmo->getTarget() == obj) {
        deleteCurrentGizmo();
    }
    hoveredIDs.erase(obj->getObjectID());
    selectedIDs.erase(obj->getObjectID());

    // Destructor already handle this
    // if (Physics::PhysicsBody* body = obj->getPhysicsBody()) {
    //     removeFromPhysicsSystem(body);
    // }

    pickableObjects.erase(
        std::remove(pickableObjects.begin(), pickableObjects.end(), static_cast<IPickable*>(obj)),
        pickableObjects.end()
    );
    scene->removeDrawable(obj);
    sceneObjectsByID.erase(obj->getObjectID());
    const std::string& objectName = obj->getName();
    auto nameIt = usedNames.find(objectName);
    if (nameIt != usedNames.end() && nameIt->second == obj) {
        usedNames.erase(nameIt);
    }
    emit objectRemoved(obj);

    auto it = std::find_if(sceneObjects.begin(), sceneObjects.end(),
    [obj](const std::unique_ptr<SceneObject>& ptr) {
        return ptr.get() == obj;
    });

    if (it != sceneObjects.end()) {
        sceneObjects.erase(it);
    }
}

void SceneManager::deleteAllObjects() {
    // Use while to not skipping elements
    while (!sceneObjects.empty()) {
        deleteObject(sceneObjects.back().get());
    }
    usedNames.clear();
    sceneObjectsByID.clear();
}

const std::vector<std::unique_ptr<SceneObject>>& SceneManager::getObjects() const {
    return sceneObjects;
}

SceneObject* SceneManager::getObjectByID(uint32_t objectID) const {
    auto it = sceneObjectsByID.find(objectID);
    return it != sceneObjectsByID.end() ? it->second : nullptr;
}

std::string SceneManager::generateDefaultName(const CreationOptions& options) {
    return std::visit([&](auto&& opt) -> std::string {
        using T = std::decay_t<decltype(opt)>;

        if constexpr (std::is_same_v<T, PointMassOptions>)
            return "Point Mass";

        if constexpr (std::is_same_v<T, RigidBodyOptions>)
            return "Rigid Body";

        return "Scene Object";
    }, options);
}

bool SceneManager::isNameUnique(const std::string &name, SceneObject *self) const {
    auto it = usedNames.find(name);

    if (it == usedNames.end())
        return true;

    return it->second == self;
}

void SceneManager::setObjectName(SceneObject *obj, const std::string &newName) {
    assert(obj);

    const std::string& oldName = obj->getName();
    if (oldName == newName)
        return;

    assert(isNameUnique(newName, obj));

    if (!oldName.empty()) {
        auto it = usedNames.find(oldName);
        if (it != usedNames.end() && it->second == obj) {
            usedNames.erase(it);
        }
    }

    obj->setName(newName);
    usedNames[newName] = obj;

    emit objectRenamed(obj, newName.data());
}

std::string SceneManager::makeUniqueName(const std::string &baseName) const {
    if (usedNames.find(baseName) == usedNames.end())
        return baseName;

    int index = 1;
    while (true) {
        std::string candidate = baseName + " (" + std::to_string(index) + ")";
        if (usedNames.find(candidate) == usedNames.end())
            return candidate;
        index++;
    }
}

void SceneManager::setCameraTarget(SceneObject* target) {
    if (scene && scene->getCamera()) {
        scene->getCamera()->setTarget(target);
    }
    if (target) {
        selectObject(target);
    }
}

void SceneManager::focusObject(SceneObject* target) {
    if (!target) return;

    selectObject(target);

    if (scene && scene->getCamera()) {
        scene->getCamera()->focusOn(target);
    }
}

void SceneManager::clearCameraTarget() {
    if (scene && scene->getCamera()) {
        scene->getCamera()->clearTarget();
    }
}

Math::Ray SceneManager::getMouseRay() {
    QPointF mousePos = window->getMousePos();
    const qreal dpr = window->devicePixelRatioF();
    const double mxd = mousePos.x() * dpr;
    const double myd = mousePos.y() * dpr;
    const QSize fbSize = window->getFramebufferSize();
    Camera* camera = scene->getCamera();

    return {
        camera->position,
        Math::screenToWorldRayDirection(
            mxd, myd,
            fbSize.width(), fbSize.height(),
            camera->front, camera->right, camera->up, camera->fov)
    };
}

void SceneManager::updateHoverState(const Math::Ray &mouseRay) {
    hoveredIDs.clear();

    IPickable* hovered = Math::findFirstHit(pickableObjects, mouseRay, currentGizmo.get())->object;
    if (hovered) {
        hoveredIDs.insert(hovered->getObjectID());
    }
}

void SceneManager::selectObject(SceneObject* obj) {
    setSelectFor(nullptr);
    if (!obj) return;

    setSelectFor(obj);
    setGizmoFor(obj, true);
    emit selectedItem(obj);
}

void SceneManager::handleMouseButton(Qt::MouseButton button, QEvent::Type type, Qt::KeyboardModifiers mods) {
    const bool isPress = (type == QEvent::MouseButtonPress);
    const bool isRelease = (type == QEvent::MouseButtonRelease);
    Camera* camera = scene->getCamera();

    if (button == Qt::RightButton) {
        if (isPress) {
            rightClickStartDir = camera->front;

            if (!window->isMouseCaptured()) {
                window->setMouseCaptured(true);
                camera->resetMouse();
            }
        }
        else if (isRelease && window->isMouseCaptured()) {
            window->setMouseCaptured(false);
            camera->resetMouse();

            if (glm::distance(camera->front, rightClickStartDir) < 0.05f) {
                Math::Ray ray = getMouseRay();
                IPickable* hit = Math::findFirstHit(pickableObjects, ray, currentGizmo.get())->object;

                if (auto* sceneObj = dynamic_cast<SceneObject*>(hit)) {
                    emit contextMenuRequested(QCursor::pos(), sceneObj);
                }
            }
        }
    }

    if (button == Qt::LeftButton) {
        Math::Ray ray = getMouseRay();
        auto hit = Math::findFirstHit(pickableObjects, ray, currentGizmo.get());
        IPickable* clickedObject = hit ? hit->object : nullptr;
        bool clickedCurrentGizmo = clickedObject && currentGizmo && (clickedObject->getObjectID() == currentGizmo->getObjectID());

        if (isPress) {
            // Clear selection if we click something new that isn't the current gizmo
            if (!clickedObject || (!selectedIDs.empty() && !clickedCurrentGizmo)) {
                setSelectFor(nullptr);
            }

            if (!clickedObject)
                return;

            clickedObject->handleClick(ray, hit->distance);
            selectedIDs.insert(clickedObject->getObjectID());
            if (!clickedCurrentGizmo) {
                emit selectedItem(dynamic_cast<SceneObject*>(clickedObject));
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
        hoveredIDs.insert(currentGizmo->getObjectID());
        Math::Ray ray = getMouseRay();
        currentGizmo->handleDrag(ray);
    }

    const float cameraDt = heldKeys.contains(Qt::Key_Shift) ? dt * 0.1f : dt;
    if (heldKeys.contains(Qt::Key_A))
        camera->processKeyboard(Movement::LEFT, cameraDt);
    if (heldKeys.contains(Qt::Key_D))
        camera->processKeyboard(Movement::RIGHT, cameraDt);
    if (heldKeys.contains(Qt::Key_W))
        camera->processKeyboard(Movement::FORWARD, cameraDt);
    if (heldKeys.contains(Qt::Key_S))
        camera->processKeyboard(Movement::BACKWARD, cameraDt);

    // TODO: Z,X,P,O should be processed by press once
    // It currently triggers every frame (caused bugs)
    if (heldKeys.contains(Qt::Key_Z)) {
        startSimulation();
    }
    if (heldKeys.contains(Qt::Key_X)) {
        stopSimulation();
    }
    if (heldKeys.contains(Qt::Key_P)) {
        if (saveScene("scene.json"))
            std::cout << "Save Success!" << std::endl;
        else
            std::cout << "Save Failed!" << std::endl;
    }
    if (heldKeys.contains(Qt::Key_O)) {
        if (loadScene("scene.json"))
            std::cout << "Load Success!" << std::endl;
        else
            std::cout << "Load Failed!" << std::endl;
    }
}

void SceneManager::setGizmoFor(SceneObject *newTarget, bool redraw) {
    if (currentGizmo) {
        if (currentGizmo->getTarget() == newTarget && redraw == false) {
            deleteCurrentGizmo();
        } else {
            deleteCurrentGizmo();
            currentGizmo = std::make_unique<Gizmo>(selectedGizmoType, this, newTarget);
        }
    } else {
        currentGizmo = std::make_unique<Gizmo>(selectedGizmoType, this, newTarget);
    }
}

void SceneManager::setSelectFor(SceneObject *obj, bool flag) {
    if (!obj) { // If passing in object as nullptr, deselect all objects
        selectedIDs.clear();
        if (currentGizmo)
            deleteCurrentGizmo();
        emit selectedItem(nullptr);
        return;
    }
    uint32_t objID = obj->getObjectID();
    if (flag) {
        if (selectedIDs.find(objID) == selectedIDs.end())
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

bool SceneManager::saveScene(const QString &file) {
    SceneSerializer serializer(this);
    return serializer.saveToJson(file);
}

bool SceneManager::loadScene(const QString &file) {
    SceneSerializer serializer(this);
    return serializer.loadFromJson(file);
}

void SceneManager::initDebugDrawables() {
    if (!scene || !window) return;

    pathTraces = std::make_unique<PathTraces>(this, window);
    forces = std::make_unique<Forces>(this);
    colliders = std::make_unique<Colliders>(this, window);

    scene->addDrawable(pathTraces.get());
    scene->addDrawable(forces.get());
    scene->addDrawable(colliders.get());

    sceneRayTracer = std::make_unique<SceneRayTracer>(this, window);
    auto& vs = AppSettings::getInstance().getGroup<GraphicsSettings>();
    sceneRayTracer->setEnabled(vs.useRayTraced);
}

void SceneManager::removeDebugDrawables() {
    if (!scene) {
        sceneRayTracer.reset();
        pathTraces.reset();
        forces.reset();
        colliders.reset();
        return;
    }

    if (pathTraces) {
        scene->removeDrawable(pathTraces.get());
        pathTraces.reset();
    }

    if (forces) {
        scene->removeDrawable(forces.get());
        forces.reset();
    }

    if (colliders) {
        scene->removeDrawable(colliders.get());
        colliders.reset();
    }

    sceneRayTracer.reset();
}

void SceneManager::applyDebugSettings() {
    auto& dbg = AppSettings::getInstance().getGroup<DebugSettings>();
    auto& vs = AppSettings::getInstance().getGroup<GraphicsSettings>();

    if (sceneRayTracer) {
        sceneRayTracer->setEnabled(vs.useRayTraced);
    }
    if (pathTraces) {
        pathTraces->setEnabled(dbg.showAllPathTrails);
        pathTraces->setTimeWindow(dbg.pathTrailTime);
    }
    if (forces) {
        forces->setEnabled(dbg.showForces);
    }
    if (colliders) {
        colliders->setEnabled(dbg.showColliders);
    }
}
