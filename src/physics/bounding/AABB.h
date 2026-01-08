#pragma once
#include <glm/glm.hpp>
#include "ICollider.h"

namespace Physics::Bounding{
    class AABB : public ICollider{
    public:
        AABB() = default;

        AABB(const glm::vec3& center, const glm::vec3& halfExtents);
        std::unique_ptr<ICollider> getTransformed(const glm::mat4 &modelMatrix) const override;

        bool intersectsAABB(const AABB& other) const;
        std::optional<float> intersectRay(const Math::Ray& ray) const override;
        bool contains(const glm::vec3& p) const override;
        ContactInfo closestPoint(const glm::vec3& p) const override;

    private:
        glm::vec3 minCorner;
        glm::vec3 maxCorner;
        glm::vec3 center;
        glm::vec3 halfExtents;
    };
}
