#pragma once
#include "IDrawable.h"

enum class Axis {
    X, Y, Z
};

class IHandle : public IDrawable{
public:
    virtual void onDrag(const glm::vec3& rayOrig, const glm::vec3& rayDir) = 0;
    virtual void setDragState(glm::vec3 initHitPos, glm::vec3 originPos) = 0;
    virtual glm::mat4 getModelMatrix() const = 0;
    virtual Mesh* getMesh() const = 0;
};
