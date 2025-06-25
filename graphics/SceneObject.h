#pragma once
#include <glm/glm.hpp>
#include <graphics/Mesh.h>

class SceneObject {
public:
    SceneObject(Mesh* meshPtr, unsigned int program);

    void draw() const;

    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::vec3& rot);
    void setScale(const glm::vec3& scl);
private:
    Mesh* mesh;
    unsigned int shaderProgram;

    glm::vec3 position {0.0f};
    glm::vec3 rotation {0.0f};
    glm::vec3 scale    {1.0f};

    glm::mat4 getModelMatrix() const;
};