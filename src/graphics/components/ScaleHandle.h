#pragma once
#include "IHandle.h"
#include <graphics/core/SceneObject.h>

class ScaleHandle : public IHandle{
public:
    ScaleHandle(SceneObject* tgt, Axis ax);

    void onDrag(const Math::Ray& ray) override;
    void setDragState(glm::vec3 initHitPos) override;
    glm::mat4 getModelMatrix() const override;
    glm::vec3 getAxisDir() const override { return axisDir(axis); }

private:
    SceneObject* target;
    Axis axis;

    constexpr static float thickness = 0.5f;
    constexpr static float length = 1.0f;

    glm::vec3 initialHitPoint;
    glm::vec3 originalScale;
};
