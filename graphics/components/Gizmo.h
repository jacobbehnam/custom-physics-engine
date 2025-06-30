#pragma once
#include <graphics/core/SceneObject.h>
#include <graphics/components/Shader.h>
#include <graphics/core/Camera.h>
#include <graphics/components/TranslateHandle.h>

class Gizmo : public IDrawable, public IPickable{
public:
    Gizmo(Scene* scene, Mesh* mesh, SceneObject* tgt, Shader* shader);

    void draw() const override;
    Shader* getShader() const override;
    bool rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance) override;
    void handleClick(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float distance) override;
    void handleRelease();
    void handleDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir);

    SceneObject* getTarget();

    //TODO: make private
    std::vector<TranslateHandle*> handles;
    bool isDragging = false;

private:
    SceneObject* target;
    TranslateHandle* activeHandle = nullptr;
};