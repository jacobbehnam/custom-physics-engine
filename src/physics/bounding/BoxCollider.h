#pragma once
#include "ICollider.h"
#include <glm/gtx/quaternion.hpp>

namespace Physics::Bounding {
    class BoxCollider : public ICollider {
    public:
        BoxCollider() = default;
        BoxCollider(const glm::vec3& center, const glm::vec3& halfExtents, const glm::quat& rotation);

        std::unique_ptr<ICollider> getTransformed(const glm::mat4 &modelMatrix) const override;

        bool contains(const glm::vec3 &p) const override;

        ContactInfo closestPoint(const glm::vec3 &p) const override;

        std::optional<float> intersectRay(const Math::Ray& ray) const override;
    private:
        glm::vec3 center{};
        glm::vec3 halfExtents{};
        glm::quat rotation{1, 0, 0, 0};
    };
}
