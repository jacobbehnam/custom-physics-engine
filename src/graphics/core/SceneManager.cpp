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
#include "ui/AppSettings.h"
#include "ui/settings/DebugSettings.h"

#include <array>
#include <algorithm>
#include <string_view>

SceneManager::SceneManager(OpenGLWindow* win, Scene *scn) : window(win), scene(scn), physicsSystem(std::make_unique<Physics::PhysicsSystem>()) {
    // TODO: preload shaders in resourcemanager (rn its in Scene)
    physicsSystem->start();
    initDebugDrawables();
}

SceneManager::~SceneManager() {
    removeDebugDrawables();
}

void SceneManager::defaultSetup() {
    constexpr double metersPerKm = 1000.0;
    constexpr double metersPerMillionKm = 1.0e9;

    struct BodySpec {
        const char* name;
        double massKg;
        double radiusKm;
        double distanceMillionKm;
        double orbitalVelocityKmS;
        double meanTempK;
        float densityKgM3;
    };

    auto makeThermal = [](double tempK, float density) {
        ThermalProperties thermal;
        thermal.tempK = tempK;
        thermal.emissivity = 0.95f;
        thermal.heatTransferCoeff = 0.0f;
        thermal.specificHeat = 1000.0f;
        thermal.conductivity = 0.0f;
        thermal.density = density;
        return thermal;
    };

    auto createSolarBody = [&](const BodySpec& spec) {
        PointMassOptions options;
        options.base.position = glm::vec3(static_cast<float>(spec.distanceMillionKm * metersPerMillionKm), 0.0f, 0.0f);
        options.base.scale = glm::vec3(static_cast<float>(spec.radiusKm * 2.0 * metersPerKm));
        options.mass = spec.massKg;
        options.velocity = glm::vec3(0.0f, 0.0f, static_cast<float>(spec.orbitalVelocityKmS * metersPerKm));

        SceneObject* body = createObject("prim_sphere", ResourceManager::getShader("basic"), options);
        setObjectName(body, spec.name);
        body->getPhysicsBody()->setThermalProperty(makeThermal(spec.meanTempK, spec.densityKgM3), BodyLock::NOLOCK);
        return body;
    };

    physicsSystem->setGlobalAcceleration(glm::vec3(0.0f));
    physicsSystem->setAmbientTemperature(2.725f);
    physicsSystem->setGravitationalConstant(Constants::G);
    setSimSpeed(10e5f);

    PointMassOptions sunOptions;
    sunOptions.base.position = glm::vec3(0.0f);
    sunOptions.base.scale = glm::vec3(696000.0f * 2.0f * static_cast<float>(metersPerKm));
    sunOptions.mass = 1.9891e30;
    SceneObject* sun = createObject("prim_sphere", ResourceManager::getShader("basic"), sunOptions);
    setObjectName(sun, "Sun");
    sun->getPhysicsBody()->setThermalProperty(makeThermal(5778.0, 1408.0f), BodyLock::NOLOCK);

    const std::array<BodySpec, 8> planets{{
        {"Mercury", 0.330e24, 4879.0 / 2.0, 57.9, 47.4, 167.0 + 273.15, 5429.0f},
        {"Venus",   4.87e24, 12104.0 / 2.0, 108.2, 35.0, 464.0 + 273.15, 5243.0f},
        {"Earth",   5.97e24, 12756.0 / 2.0, 149.6, 29.8, 15.0 + 273.15, 5514.0f},
        {"Mars",    0.642e24, 6792.0 / 2.0, 227.9, 24.1, -65.0 + 273.15, 3934.0f},
        {"Jupiter", 1898.0e24, 142984.0 / 2.0, 778.6, 13.1, -110.0 + 273.15, 1326.0f},
        {"Saturn",  568.0e24, 120536.0 / 2.0, 1433.5, 9.7, -140.0 + 273.15, 687.0f},
        {"Uranus",  86.8e24, 51118.0 / 2.0, 2872.5, 6.8, -195.0 + 273.15, 1270.0f},
        {"Neptune", 102.0e24, 49528.0 / 2.0, 4495.1, 5.4, -200.0 + 273.15, 1638.0f},
    }};

    SceneObject* earth = nullptr;
    for (const BodySpec& planet : planets) {
        SceneObject* body = createSolarBody(planet);
        if (std::string_view(planet.name) == "Earth")
            earth = body;
    }

    PointMassOptions moonOptions;
    moonOptions.base.position = glm::vec3(static_cast<float>((149.6 + 0.3844) * metersPerMillionKm), 0.0f, 0.0f);
    moonOptions.base.scale = glm::vec3(1737.4f * 2.0f * static_cast<float>(metersPerKm));
    moonOptions.mass = 0.07346e24;
    moonOptions.velocity = glm::vec3(0.0f, 0.0f, static_cast<float>((29.8 + 1.022) * metersPerKm));
    SceneObject* moon = createObject("prim_sphere", ResourceManager::getShader("basic"), moonOptions);
    setObjectName(moon, "Moon");
    moon->getPhysicsBody()->setThermalProperty(makeThermal(270.4, 3344.0f), BodyLock::NOLOCK);

    focusObject(earth);
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

    sceneObjectPtrs.push_back(ptr);
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
    sceneObjectPtrs.push_back(ptr);
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

    // Destructor already handle this
    // if (Physics::PhysicsBody* body = obj->getPhysicsBody()) {
    //     removeFromPhysicsSystem(body);
    // }

    pickableObjects.erase(
        std::remove(pickableObjects.begin(), pickableObjects.end(), static_cast<IPickable*>(obj)),
        pickableObjects.end()
    );
    scene->removeDrawable(obj);
    sceneObjectPtrs.erase(
        std::remove(sceneObjectPtrs.begin(), sceneObjectPtrs.end(), obj),
        sceneObjectPtrs.end()
    );
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
    sceneObjectPtrs.clear();
    sceneObjectsByID.clear();
}

const std::vector<SceneObject*>& SceneManager::getObjects() const {
    return sceneObjectPtrs;
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
        setSelectFor(nullptr);
        setSelectFor(target);
        setGizmoFor(target, true);
        emit selectedItem(target);
    }
}

void SceneManager::focusObject(SceneObject* target) {
    if (!target) return;

    setSelectFor(nullptr);
    setSelectFor(target);
    setGizmoFor(target, true);

    if (scene && scene->getCamera()) {
        scene->getCamera()->focusOn(target);
    }
    emit selectedItem(target);
}

void SceneManager::clearCameraTarget() {
    if (scene && scene->getCamera()) {
        scene->getCamera()->clearTarget();
    }
}

Math::Ray SceneManager::getMouseRay() {
    QPointF mousePos = window->getMousePos();
    QSize fbSize = window->getFramebufferSize();

    return {
        scene->getCamera()->position,
        Math::screenToWorldRayDirection(
            mousePos.x(), mousePos.y(),
            fbSize.width(), fbSize.height(),
            scene->getCamera()->getViewMatrix(), scene->getCamera()->getProjMatrix())
    };
}

void SceneManager::updateHoverState(const Math::Ray &mouseRay) {
    hoveredIDs.clear();

    IPickable* hovered = Math::findFirstHit(pickableObjects, mouseRay, currentGizmo.get())->object;
    if (hovered) {
        hoveredIDs.insert(hovered->getObjectID());
    }
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

    if (heldKeys.contains(Qt::Key_A))
        camera->processKeyboard(Movement::LEFT, dt);
    if (heldKeys.contains(Qt::Key_D))
        camera->processKeyboard(Movement::RIGHT, dt);
    if (heldKeys.contains(Qt::Key_W))
        camera->processKeyboard(Movement::FORWARD, dt);
    if (heldKeys.contains(Qt::Key_S))
        camera->processKeyboard(Movement::BACKWARD, dt);

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
}

void SceneManager::removeDebugDrawables() {
    if (!scene) {
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
}

void SceneManager::applyDebugSettings() {
    auto& dbg = AppSettings::getInstance().getGroup<DebugSettings>();

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
