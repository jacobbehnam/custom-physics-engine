#pragma once
#include <QObject>
#include <unordered_map>

#include "ResourceManager.h"
#include "graphics/core/Scene.h"
#include "graphics/core/SceneObjectOptions.h"
#include "ui/OpenGLWindow.h"
#include "physics/PhysicsSystem.h"

class PathTraces;
class Forces;
class Colliders;

enum class Primitive {
    CUBE,
    SPHERE
};

// TODO: make enum
struct SimulationStopCondition {
    int subjectID = -1; // -1 = No Selection, >=0 = Specific Object
    int property = 0;   // 0=PosY, 1=VelY, 2=Distance...
    int op = 0;         // 0=Less, 1=Greater
    float value = 0.0f; // The threshold
    int targetID = -1;  // If property is 'Distance', this is the other object
    glm::vec3 targetPos = glm::vec3(0.0f);
};

class SceneManager : public QObject {
    Q_OBJECT

public:
    SceneManager(OpenGLWindow* win, Scene* scene);
    ~SceneManager() override;
    SceneObject* createPrimitive(Primitive type, Shader* shader, const CreationOptions& = ObjectOptions{});
    SceneObject* createObject(const std::string &meshName, Shader* shader = ResourceManager::getShader("basic"), const CreationOptions& = ObjectOptions{});
    void deleteObject(SceneObject* obj);
    void deleteAllObjects();
    const std::vector<SceneObject*>& getObjects() const;
    SceneObject* getObjectByID(uint32_t objectID) const;
    bool isNameUnique(const std::string& name, SceneObject* self) const;
    void setObjectName(SceneObject* obj, const std::string& newName);
    std::string makeUniqueName(const std::string& baseName) const;

    void setCameraTarget(SceneObject* target);
    void focusObject(SceneObject* target);
    void clearCameraTarget();
    bool isCameraFollowing() const { return scene && scene->getCamera() && scene->getCamera()->hasTarget(); }
    const SceneObject* getCameraTarget() const { return scene && scene->getCamera() ? scene->getCamera()->getTarget() : nullptr; }

    void addToPhysicsSystem(Physics::PhysicsBody* body) const { physicsSystem->addBody(body); }
    void removeFromPhysicsSystem(Physics::PhysicsBody* body) const { physicsSystem->removeBody(body); }
    glm::vec3 getGlobalAcceleration() const { return physicsSystem->getGlobalAcceleration(); }
    void setGlobalAcceleration(const glm::vec3& newAcceleration) const { physicsSystem->setGlobalAcceleration(newAcceleration); }
    bool isPhysicsRunning() const { return physicsSystem->isPhysicsEnabled(); }
    float getSimSpeed() const { return window->getSimSpeed(); }
    void setSimSpeed(float newSpeed) { window->setSimSpeed(newSpeed); physicsSystem->setSimSpeed(newSpeed); }
    void startSimulation() const { window->setRenderClockRunning(true); physicsSystem->enablePhysics(); }
    void stopSimulation() const { physicsSystem->disablePhysics(); window->setRenderClockRunning(false); }
    double getGravitationalConstant() const { return physicsSystem->getGravitationalConstant(); }
    void setGravitationalConstant(double newG) const { physicsSystem->setGravitationalConstant(newG); }
    float getAmbientTemperature() const { return physicsSystem->getAmbientTemperature(); }
    void setAmbientTemperature(float newTemp) const { physicsSystem->setAmbientTemperature(newTemp); }
    void stepPhysics(float dt) const { physicsSystem->step(dt); }

    void addPickable(IPickable* obj) { pickableObjects.push_back(obj); }
    void addDrawable(IDrawable* obj) const { scene->addDrawable(obj); }
    void removePickable(IPickable* obj);
    void removeDrawable(IDrawable* obj) const { scene->removeDrawable(obj); }
    void updateHoverState(const Math::Ray& mouseRay);
    void selectObject(SceneObject* obj);
    void setSelectFor(SceneObject *obj, bool flag = true);

    void processHeldKeys(const QSet<int> &heldKeys, float dt);
    void handleMouseButton(Qt::MouseButton button, QEvent::Type type, Qt::KeyboardModifiers mods);
    void setGizmoFor(SceneObject *newTarget, bool redraw = false);
    void deleteCurrentGizmo();

    void applyDebugSettings();

    bool saveScene(const QString &file);
    bool loadScene(const QString &file);

    void defaultSetup(); // TODO: prob will remove later.
    std::unordered_set<uint32_t> hoveredIDs, selectedIDs; // TODO: dont make public

    Scene* scene; // TODO: move
    std::unique_ptr<Physics::PhysicsSystem> physicsSystem; // TODO: move
    SimulationStopCondition stopCondition; // TODO: move

signals:
    void objectAdded(SceneObject* obj);
    void objectRemoved(SceneObject* obj);
    void objectRenamed(SceneObject* obj, const QString& newName);

    void selectedItem(SceneObject* object); // Left click on object
    void contextMenuRequested(const QPoint& globalPos, SceneObject* object); // Right Click on object

private:
    OpenGLWindow* window;

    std::vector<std::unique_ptr<SceneObject>> sceneObjects;
    std::vector<SceneObject*> sceneObjectPtrs;
    std::unordered_map<uint32_t, SceneObject*> sceneObjectsByID;
    std::vector<IPickable*> pickableObjects;

    GizmoType selectedGizmoType = GizmoType::TRANSLATE;
    std::unique_ptr<Gizmo> currentGizmo;

    std::string generateDefaultName(const CreationOptions& creationOptions);
    std::unordered_map<std::string, SceneObject*> usedNames;

    Math::Ray getMouseRay();

    void initDebugDrawables();
    void removeDebugDrawables();

    std::unique_ptr<PathTraces> pathTraces;
    std::unique_ptr<Forces> forces;
    std::unique_ptr<Colliders> colliders;

    // To track if a right click was a click or drag
    glm::vec3 rightClickStartDir;
};
