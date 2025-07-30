#pragma once
#include <QObject>
#include "graphics/core/Scene.h"

enum class Primitive {
    CUBE,
    SPHERE
};

class SceneManager : public QObject {
    Q_OBJECT

public:
    SceneManager(OpenGLWindow* win, Scene* scene);
    SceneObject* createPrimitive(Primitive type, Shader* shader, bool wantPhysics);
    void deleteObject(SceneObject* obj);

    void addToPhysicsSystem(IPhysicsBody* body) const { physicsSystem->addBody(body); }
    void removeFromPhysicsSystem(IPhysicsBody* body) const { physicsSystem->removeBody(body); }
    glm::vec3 getGlobalAcceleration() const { return physicsSystem->getGlobalAcceleration(); }
    void setGlobalAcceleration(const glm::vec3& newAcceleration) { physicsSystem->setGlobalAcceleration(newAcceleration); }
    void stepPhysics(float dt) const { physicsSystem->step(dt); }

    void addPickable(IPickable* obj) { pickableObjects.push_back(obj); }
    void addDrawable(IDrawable* obj) { scene->addDrawable(obj); }
    void removePickable(IPickable* obj);
    void removeDrawable(IDrawable* obj) { scene->removeDrawable(obj); }
    void updateHoverState(const MathUtils::Ray& mouseRay);
    void setSelectFor(SceneObject *obj, bool flag);

    void processHeldKeys(const QSet<int> &heldKeys, float dt);
    void handleMouseButton(Qt::MouseButton button, QEvent::Type type, Qt::KeyboardModifiers mods);
    void setGizmoFor(SceneObject *newTarget, bool redraw = false);
    void deleteCurrentGizmo();

    void defaultSetup(); // TODO: prob will remove later.
    std::unordered_set<uint32_t> hoveredIDs, selectedIDs; // TODO: dont make public

    Scene* scene; // TODO: move

signals:
    void objectAdded(SceneObject* obj);
    void objectRemoved(SceneObject* obj);
    void objectRenamed(SceneObject* obj, const QString& oldName);

    void selectedItem(SceneObject* object);

private:
    std::unique_ptr<Physics::PhysicsSystem> physicsSystem;

    OpenGLWindow* window;

    std::vector<std::unique_ptr<SceneObject>> sceneObjects;
    std::vector<IPickable*> pickableObjects;

    GizmoType selectedGizmoType = GizmoType::TRANSLATE;
    std::unique_ptr<Gizmo> currentGizmo;

    MathUtils::Ray getMouseRay();
};
