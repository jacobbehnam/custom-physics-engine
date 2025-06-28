#pragma once
#include <graphics/SceneObject.h>
#include <graphics/Shader.h>
#include <graphics/Camera.h>
#include <GLFW/glfw3.h>
#include <graphics/TranslateHandle.h>

class Gizmo : public IDrawable, public IPickable{
public:
    Gizmo(Scene* scene, Mesh* mesh, SceneObject* tgt, Shader* shader);

    void draw() const override;
    Shader* getShader() const override;
    bool rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance) override;
    void handleClick(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float distance) override;
    void handleDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir);

    //TODO: make private
    std::vector<TranslateHandle*> handles;

private:
    SceneObject* target;
    TranslateHandle* activeHandle = nullptr;
};