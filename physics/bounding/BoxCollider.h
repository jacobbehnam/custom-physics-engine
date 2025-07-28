#pragma once
#include "graphics/interfaces/ICollider.h"
#include <glm/gtx/quaternion.hpp>

namespace Physics::Bounding {
    class BoxCollider : public ICollider {
    public:
        BoxCollider() = default;
        BoxCollider(const glm::vec3& center, const glm::vec3& halfExtents, const glm::quat& rotation);

        BoxCollider* getTransformed(const glm::mat4 &modelMatrix) const override;

        bool contains(const glm::vec3 &p) const override;

        ContactInfo closestPoint(const glm::vec3 &p) const override;

        bool intersectRay(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float &outT) const override;
    private:
        glm::vec3 center{};
        glm::vec3 halfExtents{};
        glm::quat rotation{1, 0, 0, 0};
    };
}
