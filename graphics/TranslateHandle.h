#pragma once
#include <graphics/IDrawable.h>
#include <graphics/Mesh.h>
#include <glm/glm.hpp>
#include <graphics/Shader.h>

enum class Axis {
    X, Y, Z
};

class SceneObject;

class TranslateHandle : IDrawable{
public:
    TranslateHandle(Mesh* m, Shader* sdr, SceneObject* tgt, Axis ax);

    void draw() const override;

    Shader* getShader() const;
private:
    Mesh* mesh;
    Shader* shader;
    SceneObject* target;
    Axis axis;

    float length = 1.0f;
    float thickness = 0.05f;

    glm::mat4 getModelMatrix() const;
};
