#pragma once
#include <graphics/components/Mesh.h>
#include <glm/glm.hpp>
#include <graphics/components/Shader.h>

#include "graphics/interfaces/IHandle.h"

class SceneObject;

class TranslateHandle : public IHandle{
public:
    TranslateHandle(Mesh* m, Shader* sdr, SceneObject* tgt, Axis ax);

    void draw() const override;
    void onDrag(const glm::vec3& rayOrig, const glm::vec3& rayDir) override;
    void setDragState(glm::vec3 initHitPos, glm::vec3 originPos) override;

    Shader* getShader() const override;
    Mesh* getMesh() const override;

    glm::mat4 getModelMatrix() const override;
private:
    Mesh* mesh;
    Shader* shader;
    SceneObject* target;
    Axis axis;

    float length = 1.0f;
    float thickness = 0.5f;

    glm::vec3 initialHitPoint;
    glm::vec3 originalPosition;
};
