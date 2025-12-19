#pragma once
#include <QObject>
#include "graphics/core/Scene.h"
#include "graphics/core/SceneObjectOptions.h"
#include "ui/OpenGLWindow.h"

enum class Primitive {
    CUBE,
    SPHERE
};

class SceneManager : public QObject {
    Q_OBJECT

public:
    SceneManager(OpenGLWindow* win, Scene* scene);
    SceneObject* createPrimitive(Primitive type, Shader* shader, const CreationOptions& = ObjectOptions{});
    SceneObject* createObject(const std::string &meshName, Shader* shader, const CreationOptions& = ObjectOptions{});
    void deleteObject(SceneObject* obj);
    void deleteAllObjects();
    std::vector<SceneObject*> getObjects() const;

    void addToPhysicsSystem(Physics::PhysicsBody* body) const { physicsSystem->addBody(body); }
    void removeFromPhysicsSystem(Physics::PhysicsBody* body) const { physicsSystem->removeBody(body); }
    glm::vec3 getGlobalAcceleration() const { return physicsSystem->getGlobalAcceleration(); }
    void setGlobalAcceleration(const glm::vec3& newAcceleration) const { physicsSystem->setGlobalAcceleration(newAcceleration); }
    float getSimSpeed() const { return window->getSimSpeed(); }
    void setSimSpeed(float newSpeed) { window->setSimSpeed(newSpeed); physicsSystem->setSimSpeed(newSpeed); }
    void stepPhysics(float dt) const { physicsSystem->step(dt); }

    void addPickable(IPickable* obj) { pickableObjects.push_back(obj); }
    void addDrawable(IDrawable* obj) const { scene->addDrawable(obj); }
    void removePickable(IPickable* obj);
    void removeDrawable(IDrawable* obj) const { scene->removeDrawable(obj); }
    void updateHoverState(const MathUtils::Ray& mouseRay);
    void setSelectFor(SceneObject *obj, bool flag = true);

    void processHeldKeys(const QSet<int> &heldKeys, float dt);
    void handleMouseButton(Qt::MouseButton button, QEvent::Type type, Qt::KeyboardModifiers mods);
    void setGizmoFor(SceneObject *newTarget, bool redraw = false);
    void deleteCurrentGizmo();

    bool saveScene(const QString &file);
    bool loadScene(const QString &file);

    void defaultSetup(); // TODO: prob will remove later.
    std::unordered_set<uint32_t> hoveredIDs, selectedIDs; // TODO: dont make public

    Scene* scene; // TODO: move
    std::unique_ptr<Physics::PhysicsSystem> physicsSystem; // TODO: move

signals:
    void objectAdded(SceneObject* obj);
    void objectRemoved(SceneObject* obj);
    void objectRenamed(SceneObject* obj, const QString& oldName);

    void selectedItem(SceneObject* object); // Left click on object
    void contextMenuRequested(const QPoint& globalPos, SceneObject* object); // Right Click on object

private:
    OpenGLWindow* window;

    std::vector<std::unique_ptr<SceneObject>> sceneObjects;
    std::vector<IPickable*> pickableObjects;

    GizmoType selectedGizmoType = GizmoType::TRANSLATE;
    std::unique_ptr<Gizmo> currentGizmo;

    MathUtils::Ray getMouseRay();

    // To track if a right click was a click or drag
    glm::vec3 rightClickStartDir;
};
