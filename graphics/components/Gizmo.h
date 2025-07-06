#pragma once
#include <graphics/core/SceneObject.h>
#include <graphics/components/Shader.h>
#include <graphics/core/Camera.h>
#include <graphics/interfaces/IHandle.h>

enum class GizmoType {
    TRANSLATE,
    ROTATE,
    SCALE
};

class Gizmo : public IDrawable, public IPickable{
public:
    Gizmo(GizmoType type, Scene* scene, Mesh* mesh, SceneObject* tgt, Shader* shader);
    ~Gizmo();

    void draw() const override;
    Shader* getShader() const override;
    bool rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance) override;
    void handleClick(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float distance) override;
    void setHovered(bool hovered) override;
    bool getHovered() override;
    uint32_t getObjectID() const override;
    void handleRelease();
    void handleDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir);
    Mesh* getMesh() const override {return handles[0]->getMesh();}
    glm::mat4 getModelMatrix() const override {return glm::mat4(1.0f);}

    SceneObject* getTarget();

    //TODO: make private
    std::vector<IHandle*> handles;
    bool isDragging = false;

private:
    Scene* ownerScene;
    SceneObject* target;
    IHandle* activeHandle = nullptr;

    bool isHovered = false;

    uint32_t objectID;
};