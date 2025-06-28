#pragma once
#include <graphics/IDrawable.h>
#include <graphics/Mesh.h>
#include <glm/glm.hpp>
#include <graphics/Shader.h>

enum class Axis {
    X, Y, Z
};

class SceneObject;

class TranslateHandle : public IDrawable{
public:
    TranslateHandle(Mesh* m, Shader* sdr, SceneObject* tgt, Axis ax);

    void draw() const override;
    void onDrag(const glm::vec3& rayOrig, const glm::vec3& rayDir);

    Shader* getShader() const override;
    Mesh* getMesh() const;

    glm::mat4 getModelMatrix() const;
    glm::vec3 initialHitPoint;
    glm::vec3 originalPosition;
private:
    Mesh* mesh;
    Shader* shader;
    SceneObject* target;
    Axis axis;

    float length = 1.0f;
    float thickness = 0.05f;
};
