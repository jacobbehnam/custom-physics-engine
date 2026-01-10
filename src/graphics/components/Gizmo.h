#pragma once
#include <graphics/components/Shader.h>
#include <graphics/core/Camera.h>
#include "IHandle.h"

#include "../core/IPickable.h"

class SceneManager;
class SceneObject;
class Scene;

enum class GizmoType {
    TRANSLATE,
    ROTATE,
    SCALE
};

class Gizmo : public ICustomDrawable, public IPickable{
public:
    Gizmo(GizmoType type, SceneManager* sceneManager, SceneObject* tgt);
    ~Gizmo();

    // ICustomDrawable
    void draw() const override;
    uint32_t getObjectID() const override;
    Mesh* getMesh() const override { return handleMesh; }
    Shader* getShader() const override;

    bool rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance) override;
    void handleClick(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float distance) override;
    void setHovered(bool hovered) override;
    bool getHovered() override;
    void handleRelease();
    void handleDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir);

    SceneObject* getTarget() { return target; }
    IHandle* getActiveHandle() { return activeHandle; }
    bool getIsDragging() { return isDragging; }

private:
    Scene* ownerScene;
    SceneObject* target;
    std::vector<IHandle*> handles;
    IHandle* activeHandle = nullptr;

    Shader* shader;
    Mesh* handleMesh = nullptr;

    uint32_t objectID;
    bool isDragging = false;
    bool isHovered = false;
};