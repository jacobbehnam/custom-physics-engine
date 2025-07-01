#pragma once
#include <glm/glm.hpp>
#include <graphics/components/Mesh.h>
#include <graphics/interfaces/IDrawable.h>
#include <graphics/components/Shader.h>

#include "graphics/interfaces/IPickable.h"

class Scene;

class SceneObject : public IDrawable, public IPickable{
public:
    SceneObject(Scene* scene, Mesh* meshPtr, Shader *sdr);

    void draw() const override;

    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::vec3& rot);
    void setScale(const glm::vec3& scl);
    glm::vec3 getPosition() const;
    glm::vec3 getRotation() const;

    Shader* getShader() const override;
    bool rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance) override;

    void handleClick(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float distance) override;

private:
    Mesh* mesh;
    Shader* shader;
    Scene* ownerScene;

    glm::vec3 position {0.0f};
    glm::vec3 rotation {0.0f};
    glm::vec3 scale    {1.0f};

    glm::mat4 getModelMatrix() const;
};