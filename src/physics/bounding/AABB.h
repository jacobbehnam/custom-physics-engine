#pragma once
#include <glm/glm.hpp>

#include "ICollider.h"

namespace Physics::Bounding{
    class AABB : public ICollider{
    public:
        AABB() = default;

        AABB(const glm::vec3& center, const glm::vec3& halfExtents);
        AABB* getTransformed(const glm::mat4 &modelMatrix) const override;

        bool intersectsAABB(const AABB& other) const;
        bool intersectRay(const glm::vec3& orig, const glm::vec3& dir, float& outT) const override;
        bool contains(const glm::vec3& p) const override;
        ContactInfo closestPoint(const glm::vec3& p) const override;

    private:
        glm::vec3 minCorner;
        glm::vec3 maxCorner;
        glm::vec3 center;
        glm::vec3 halfExtents;
    };
}
