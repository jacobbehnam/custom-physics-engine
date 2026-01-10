#pragma once
#include <glm/glm.hpp>
#include "IHandle.h"

class SceneObject;

class TranslateHandle : public IHandle{
public:
    TranslateHandle(SceneObject* tgt, Axis ax);

    void onDrag(const Math::Ray& ray) override;
    void setDragState(glm::vec3 initHitPos) override;
    glm::vec3 getAxisDir() const override { return axisDir(axis); }
    glm::mat4 getModelMatrix() const override;
private:
    SceneObject* target;
    Axis axis;

    constexpr static float length = 1.0f;
    constexpr static float thickness = 0.5f;

    glm::vec3 initialHitPoint;
    glm::vec3 originalPosition;
};
