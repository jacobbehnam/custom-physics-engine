#pragma once
#include <glm/glm.hpp>

namespace Physics::Bounding {
    class AABB {
    public:
        AABB() = default;

        AABB(const glm::vec3& center, const glm::vec3& halfExtents);
        AABB getTransformed(const glm::mat4& modelMatrix) const;

        bool intersectsAABB(const AABB& other) const;
        bool intersectsRay(const glm::vec3& orig, const glm::vec3& dir, float& outT) const;

    private:
        glm::vec3 minCorner;
        glm::vec3 maxCorner;
        glm::vec3 center;
        glm::vec3 halfExtents;
    };
}