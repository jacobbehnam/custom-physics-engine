#pragma once
#include <glm/glm.hpp>
#include <graphics/Mesh.h>
#include <graphics/IDrawable.h>
#include <graphics/Shader.h>

class Scene;

class SceneObject : public IDrawable{
public:
    SceneObject(Scene* scene, Mesh* meshPtr, Shader *sdr);

    void draw() const override;

    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::vec3& rot);
    void setScale(const glm::vec3& scl);
    glm::vec3 getPosition() const;
    Shader* getShader() const;

    float getBoundingRadius() const;
private:
    Mesh* mesh;
    Shader* shader;
    Scene* ownerScene;

    glm::vec3 position {0.0f};
    glm::vec3 rotation {0.0f};
    glm::vec3 scale    {1.0f};

    float boundingRadius;

    glm::mat4 getModelMatrix() const;
};