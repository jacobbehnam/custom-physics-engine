#pragma once
#include <QObject>
#include "graphics/core/Scene.h"

class SceneManager : public QObject {
    Q_OBJECT

public:
    explicit SceneManager(Scene* scene);
    SceneObject* createPrimitive(Primitive type, Shader* shader, bool wantPhysics, const glm::vec3& initPos = glm::vec3(0.0f));
    void deleteObject(SceneObject* obj);
    QVector<SceneObject*> getSceneObjects() const { return sceneObjects; }

signals:
    void objectAdded(SceneObject* obj);
    void objectRemoved(SceneObject* obj);
    void objectRenamed(SceneObject* obj, const QString& oldName);

private:
    Scene* scene;
    QVector<SceneObject*> sceneObjects;
};
