#pragma once
#include <glm/glm.hpp>
#include <graphics/Mesh.h>
#include <graphics/IDrawable.h>
#include "Shader.h"

class SceneObject : public IDrawable{
public:
    SceneObject(Mesh* meshPtr, Shader *sdr);

    void draw() const override;

    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::vec3& rot);
    void setScale(const glm::vec3& scl);
    glm::vec3 getPosition() const;

    float getBoundingRadius() const;
private:
    Mesh* mesh;
    Shader* shader;

    glm::vec3 position {0.0f};
    glm::vec3 rotation {0.0f};
    glm::vec3 scale    {1.0f};

    float boundingRadius;

    glm::mat4 getModelMatrix() const;
};

inline std::vector<SceneObject*> sceneObjects;