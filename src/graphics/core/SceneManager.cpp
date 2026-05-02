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
#include <cmath>
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
    constexpr double metersPerAu = 149597870700.0;
    constexpr double defaultEpochJd = 2461161.5;
    constexpr double j2000Jd = 2451545.0;
    constexpr double julianCenturyDays = 36525.0;
    constexpr double pi = 3.14159265358979323846264338327950288;
    constexpr double degToRad = pi / 180.0;

    struct OrbitalElements {
        double semiMajorAxisAu;
        double semiMajorAxisAuPerCentury;
        double eccentricity;
        double eccentricityPerCentury;
        double inclinationDeg;
        double inclinationDegPerCentury;
        double meanLongitudeDeg;
        double meanLongitudeDegPerCentury;
        double longitudePerihelionDeg;
        double longitudePerihelionDegPerCentury;
        double longitudeAscendingNodeDeg;
        double longitudeAscendingNodeDegPerCentury;
    };

    struct OrbitalState {
        glm::vec3 position;
        glm::vec3 velocity;
    };

    struct BodySpec {
        const char* name;
        double massKg;
        double radiusKm;
        ThermalProperties thermal;
        OrbitalElements orbit;
    };

    constexpr double sunMassKg = 1.9885e30;
    constexpr double sunRadiusKm = 695700.0;
    constexpr double sunTempK = 5772.0;

    auto makeThermal = [](double tempK, float density, float emissivity, float absorptivity, float specificHeat, float thermalMassFraction, float conductivity, float meltingPoint, double internalHeatPower = 0.0) {
        ThermalProperties thermal;
        thermal.tempK = tempK;
        thermal.internalHeatPower = internalHeatPower;
        thermal.emissivity = emissivity;
        thermal.absorptivity = absorptivity;
        thermal.heatTransferCoeff = 0.0f;
        thermal.specificHeat = specificHeat;
        thermal.thermalMassFraction = thermalMassFraction;
        thermal.conductivity = conductivity;
        thermal.density = density;
        thermal.meltingPoint = meltingPoint;
        return thermal;
    };

    auto emittedRadiationPower = [](double radiusKm, double tempK, double emissivity) {
        const double radiusM = radiusKm * 1000.0;
        const double surfaceArea = 4.0 * pi * radiusM * radiusM;
        return emissivity * Constants::STEFAN_BOLTZMANN * surfaceArea * std::pow(tempK, 4.0);
    };

    const double sunLuminosity = emittedRadiationPower(sunRadiusKm, sunTempK, 1.0);

    auto absorbedSolarPower = [&](double radiusKm, double orbitDistanceAu, double absorptivity) {
        const double radiusM = radiusKm * 1000.0;
        const double projectedArea = pi * radiusM * radiusM;
        const double orbitDistanceM = orbitDistanceAu * metersPerAu;
        const double irradiance = sunLuminosity / (4.0 * pi * orbitDistanceM * orbitDistanceM);
        return absorptivity * irradiance * projectedArea;
    };

    auto equilibriumInternalHeatPower = [&](double radiusKm, double orbitDistanceAu, double tempK, double emissivity, double absorptivity) {
        const double emitted = emittedRadiationPower(radiusKm, tempK, emissivity);
        const double absorbed = absorbedSolarPower(radiusKm, orbitDistanceAu, absorptivity);
        return std::max(0.0, emitted - absorbed);
    };

    auto normalizeRadians = [](double angle) {
        constexpr double twoPi = 6.28318530717958647692528676655900576;
        angle = std::fmod(angle, twoPi);
        if (angle < -3.14159265358979323846264338327950288) angle += twoPi;
        if (angle > 3.14159265358979323846264338327950288) angle -= twoPi;
        return angle;
    };

    auto solveEccentricAnomaly = [&](double meanAnomalyRad, double eccentricity) {
        double e = meanAnomalyRad + eccentricity * std::sin(meanAnomalyRad);
        for (int i = 0; i < 12; ++i) {
            const double delta = (meanAnomalyRad - (e - eccentricity * std::sin(e))) / (1.0 - eccentricity * std::cos(e));
            e += delta;
            if (std::abs(delta) <= 1.0e-12) break;
        }
        return e;
    };

    auto rotateOrbitToEcliptic = [](const glm::dvec3& v, double omega, double node, double inclination) {
        const double cosOmega = std::cos(omega);
        const double sinOmega = std::sin(omega);
        const double cosNode = std::cos(node);
        const double sinNode = std::sin(node);
        const double cosI = std::cos(inclination);
        const double sinI = std::sin(inclination);

        return glm::dvec3(
            (cosOmega * cosNode - sinOmega * sinNode * cosI) * v.x + (-sinOmega * cosNode - cosOmega * sinNode * cosI) * v.y,
            (cosOmega * sinNode + sinOmega * cosNode * cosI) * v.x + (-sinOmega * sinNode + cosOmega * cosNode * cosI) * v.y,
            (sinOmega * sinI) * v.x + (cosOmega * sinI) * v.y
        );
    };

    auto eclipticToEngine = [](const glm::dvec3& v) {
        return glm::vec3(static_cast<float>(v.x), static_cast<float>(v.z), static_cast<float>(v.y));
    };

    auto stateFromElements = [&](const OrbitalElements& elements, double centralMassKg, double epochJd) {
        const double centuries = (epochJd - j2000Jd) / julianCenturyDays;
        const double a = (elements.semiMajorAxisAu + elements.semiMajorAxisAuPerCentury * centuries) * metersPerAu;
        const double e = elements.eccentricity + elements.eccentricityPerCentury * centuries;
        const double inclination = (elements.inclinationDeg + elements.inclinationDegPerCentury * centuries) * degToRad;
        const double meanLongitude = (elements.meanLongitudeDeg + elements.meanLongitudeDegPerCentury * centuries) * degToRad;
        const double longitudePerihelion = (elements.longitudePerihelionDeg + elements.longitudePerihelionDegPerCentury * centuries) * degToRad;
        const double node = (elements.longitudeAscendingNodeDeg + elements.longitudeAscendingNodeDegPerCentury * centuries) * degToRad;
        const double omega = longitudePerihelion - node;
        const double meanAnomaly = normalizeRadians(meanLongitude - longitudePerihelion);
        const double eccentricAnomaly = solveEccentricAnomaly(meanAnomaly, e);
        const double cosE = std::cos(eccentricAnomaly);
        const double sinE = std::sin(eccentricAnomaly);
        const double oneMinusECosE = 1.0 - e * cosE;
        const double sqrtOneMinusESq = std::sqrt(1.0 - e * e);
        const double meanMotion = std::sqrt(Constants::G * centralMassKg / (a * a * a));
        const double eccentricAnomalyRate = meanMotion / oneMinusECosE;

        const glm::dvec3 orbitalPosition(a * (cosE - e), a * sqrtOneMinusESq * sinE, 0.0);
        const glm::dvec3 orbitalVelocity(-a * sinE * eccentricAnomalyRate, a * sqrtOneMinusESq * cosE * eccentricAnomalyRate, 0.0);

        return OrbitalState{
            eclipticToEngine(rotateOrbitToEcliptic(orbitalPosition, omega, node, inclination)),
            eclipticToEngine(rotateOrbitToEcliptic(orbitalVelocity, omega, node, inclination))
        };
    };

    auto createSolarBody = [&](const BodySpec& spec) {
        const OrbitalState state = stateFromElements(spec.orbit, sunMassKg, defaultEpochJd);
        PointMassOptions options;
        options.base.position = state.position;
        options.base.scale = glm::vec3(static_cast<float>(spec.radiusKm * 2.0 * metersPerKm));
        options.mass = spec.massKg;
        options.velocity = state.velocity;

        SceneObject* body = createObject("prim_sphere", ResourceManager::getShader("basic"), options);
        setObjectName(body, spec.name);
        body->getPhysicsBody()->setThermalProperty(spec.thermal, BodyLock::NOLOCK);
        return body;
    };

    physicsSystem->setGlobalAcceleration(glm::vec3(0.0f));
    physicsSystem->setAmbientTemperature(2.725f);
    physicsSystem->setGravitationalConstant(Constants::G);
    setSimSpeed(10e5f);

    PointMassOptions sunOptions;
    sunOptions.base.position = glm::vec3(0.0f);
    sunOptions.base.scale = glm::vec3(static_cast<float>(sunRadiusKm * 2.0 * metersPerKm));
    sunOptions.mass = sunMassKg;
    SceneObject* sun = createObject("prim_sphere", ResourceManager::getShader("basic"), sunOptions);
    setObjectName(sun, "Sun");
    sun->getPhysicsBody()->setThermalProperty(makeThermal(sunTempK, 1408.0f, 1.0f, 1.0f, 20780.0f, 1.0f, 1.0e4f, 0.0f, sunLuminosity), BodyLock::NOLOCK);

    const std::array<BodySpec, 8> planets{{
        {"Mercury", 0.33010e24, 2439.7, makeThermal(440.15, 5429.0f, 0.90f, 0.91f, 800.0f, 1.0e-8f, 2.0f, 1700.0f), {0.38709927, 0.00000037, 0.20563593, 0.00001906, 7.00497902, -0.00594749, 252.25032350, 149472.67411175, 77.45779628, 0.16047689, 48.33076593, -0.12534081}},
        {"Venus",   4.8673e24, 6051.8, makeThermal(737.15, 5243.0f, 0.01f, 0.25f, 850.0f, 5.0e-6f, 2.0f, 1700.0f, equilibriumInternalHeatPower(6051.8, 0.72333566, 737.15, 0.01, 0.25)), {0.72333566, 0.00000390, 0.00677672, -0.00004107, 3.39467605, -0.00078890, 181.97909950, 58517.81538729, 131.60246718, 0.00268329, 76.67984255, -0.27769418}},
        {"Earth",   5.9724e24, 6371.0, makeThermal(288.15, 5514.0f, 0.61f, 0.70f, 1000.0f, 1.0e-5f, 2.5f, 1700.0f), {1.00000261, 0.00000562, 0.01671123, -0.00004392, -0.00001531, -0.01294668, 100.46457166, 35999.37244981, 102.93768193, 0.32327364, 0.0, 0.0}},
        {"Mars",    0.64169e24, 3389.5, makeThermal(208.15, 3934.0f, 0.85f, 0.75f, 750.0f, 1.0e-7f, 0.08f, 1700.0f), {1.52371034, 0.00001847, 0.09339410, 0.00007882, 1.84969142, -0.00813131, -4.55343205, 19140.30268499, -23.94362959, 0.44441088, 49.55953891, -0.29257343}},
        {"Jupiter", 1898.13e24, 69911.0, makeThermal(163.15, 1326.0f, 0.55f, 0.50f, 13000.0f, 1.0e-6f, 0.18f, 0.0f, equilibriumInternalHeatPower(69911.0, 5.20288700, 163.15, 0.55, 0.50)), {5.20288700, -0.00011607, 0.04838624, -0.00013253, 1.30439695, -0.00183714, 34.39644051, 3034.74612775, 14.72847983, 0.21252668, 100.47390909, 0.20469106}},
        {"Saturn",  568.32e24, 58232.0, makeThermal(133.15, 687.0f, 0.45f, 0.66f, 13000.0f, 1.0e-6f, 0.18f, 0.0f, equilibriumInternalHeatPower(58232.0, 9.53667594, 133.15, 0.45, 0.66)), {9.53667594, -0.00125060, 0.05386179, -0.00050991, 2.48599187, 0.00193609, 49.95424423, 1222.49362201, 92.59887831, -0.41897216, 113.66242448, -0.28867794}},
        {"Uranus",  86.811e24, 25362.0, makeThermal(78.15, 1270.0f, 0.50f, 0.70f, 9000.0f, 1.0e-6f, 0.6f, 0.0f, equilibriumInternalHeatPower(25362.0, 19.18916464, 78.15, 0.50, 0.70)), {19.18916464, -0.00196176, 0.04725744, -0.00004397, 0.77263783, -0.00242939, 313.23810451, 428.48202785, 170.95427630, 0.40805281, 74.01692503, 0.04240589}},
        {"Neptune", 102.409e24, 24622.0, makeThermal(73.15, 1638.0f, 0.65f, 0.70f, 9000.0f, 1.0e-6f, 0.6f, 0.0f, equilibriumInternalHeatPower(24622.0, 30.06992276, 73.15, 0.65, 0.70)), {30.06992276, 0.00026291, 0.00859048, 0.00005105, 1.77004347, 0.00035372, -55.12002969, 218.45945325, 44.96476227, -0.32241464, 131.78422574, -0.00508664}},
    }};

    SceneObject* earth = nullptr;
    OrbitalState earthState{};
    for (const BodySpec& planet : planets) {
        SceneObject* body = createSolarBody(planet);
        if (std::string_view(planet.name) == "Earth") {
            earth = body;
            earthState.position = body->getPosition();
            earthState.velocity = body->getPhysicsBody()->getVelocity(BodyLock::NOLOCK);
        }
    }

    const OrbitalState moonRelativeState = stateFromElements(
        {0.002569555, 0.0, 0.0549, 0.0, 5.1454, 0.0, 558.5516, 481267.88088047254, 443.1862, 4069.01334885, 125.1228, -1934.1378481575},
        5.9724e24,
        defaultEpochJd
    );
    PointMassOptions moonOptions;
    moonOptions.base.position = earthState.position + moonRelativeState.position;
    moonOptions.base.scale = glm::vec3(1737.4f * 2.0f * static_cast<float>(metersPerKm));
    moonOptions.mass = 0.07346e24;
    moonOptions.velocity = earthState.velocity + moonRelativeState.velocity;
    SceneObject* moon = createObject("prim_sphere", ResourceManager::getShader("basic"), moonOptions);
    setObjectName(moon, "Moon");
    moon->getPhysicsBody()->setThermalProperty(makeThermal(270.4, 3344.0f, 0.95f, 0.88f, 741.0f, 1.0e-8f, 0.001f, 1700.0f), BodyLock::NOLOCK);

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
