#pragma once
#include "IDrawable.h"

enum class Axis {
    X, Y, Z
};

static glm::vec3 axisDir(Axis a) {
    switch (a) {
        case Axis::X:
            return {1,0,0};
        case Axis::Y:
            return {0,1,0};
        default:
            return {0,0,1};
    }
}

class Mesh;

class IHandle : public IDrawable{
public:
    virtual void onDrag(const glm::vec3& rayOrig, const glm::vec3& rayDir) = 0;
    virtual void setDragState(glm::vec3 initHitPos) = 0;
    virtual glm::vec3 getAxisDir() const = 0;
    glm::mat4 getModelMatrix() const override = 0;
    Mesh* getMesh() const override = 0;
};
