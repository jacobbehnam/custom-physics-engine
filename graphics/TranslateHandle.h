#pragma once
#include <graphics/IDrawable.h>
#include <graphics/Mesh.h>
#include <glm/glm.hpp>

enum class Axis {
    X, Y, Z
};

class SceneObject;

class TranslateHandle : IDrawable{
public:
    TranslateHandle(Mesh* m, unsigned int program, SceneObject* tgt, Axis ax);

    void draw() const override;
private:
    Mesh* mesh;
    unsigned int shaderProgram;
    SceneObject* target;
    Axis axis;

    float length = 1.0f;
    float thickness = 0.05f;

    glm::mat4 getModelMatrix() const;
};
