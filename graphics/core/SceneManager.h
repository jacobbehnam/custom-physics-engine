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
    explicit SceneManager(Scene* scene);
    SceneObject* createPrimitive(Primitive type, Shader* shader, bool wantPhysics, const glm::vec3& initPos = glm::vec3(0.0f));
    void deleteObject(SceneObject* obj);
    std::vector<SceneObject*> getSceneObjects() const { return sceneObjects; }

    void addPickable(IPickable* obj) { pickableObjects.push_back(obj); }
    void addDrawable(IDrawable* obj) { scene->addDrawable(obj); }
    void updateHoverState(const MathUtils::Ray& mouseRay);

    void defaultSetup(); // TODO: prob will remove later.
    std::unordered_set<uint32_t> hoveredIDs, selectedIDs; // TODO: dont make public

signals:
    void objectAdded(SceneObject* obj);
    void objectRemoved(SceneObject* obj);
    void objectRenamed(SceneObject* obj, const QString& oldName);

private:
    Scene* scene;
    std::vector<SceneObject*> sceneObjects;
    std::vector<IPickable*> pickableObjects;
};
