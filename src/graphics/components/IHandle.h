#pragma once
#include "../core/IDrawable.h"
#include <glm/gtc/quaternion.hpp>

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

class IHandle{
public:
    virtual ~IHandle() = default;

    virtual void onDrag(const glm::vec3& rayOrig, const glm::vec3& rayDir) = 0;
    virtual void setDragState(glm::vec3 initHitPos) = 0;
    virtual glm::vec3 getAxisDir() const = 0;
    virtual glm::mat4 getModelMatrix() const = 0;

    virtual uint32_t getObjectID() const = 0;

protected:
    IHandle() = default;

    // Helper: compute rotation matrix aligning +Y axis to the handle axis
    static glm::mat4 rotateFromYToAxis(Axis axis) {
        glm::vec3 from = {0,1,0};
        glm::vec3 to = axisDir(axis);
        glm::vec3 crossA = glm::cross(from, to);
        float cosA = glm::dot(from, to);
        if (glm::length(crossA) < 1e-3f) return glm::mat4(1.0f);
        float angle = acos(glm::clamp(cosA, -1.0f, 1.0f));
        return glm::rotate(glm::mat4(1.0f), angle, glm::normalize(crossA));
    }
};
