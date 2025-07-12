#include "AABB.h"
#include <algorithm>

Physics::Bounding::AABB::AABB(const glm::vec3 &ctr, const glm::vec3 &halfExt)
    : center(ctr), halfExtents(halfExt), minCorner(ctr-halfExt), maxCorner(ctr+halfExt) {}

Physics::Bounding::AABB Physics::Bounding::AABB::getTransformed(const glm::mat4 &modelMatrix) const {
    auto L = glm::mat3(modelMatrix);
    auto T = glm::vec3(modelMatrix[3]);

    glm::mat3 absL = glm::mat3(
        glm::abs(L[0]),
        glm::abs(L[1]),
        glm::abs(L[2])
    );

    glm::vec3 newHalfExtents = absL * halfExtents;
    glm::vec3 newCenter = L * center + T;

    return AABB(newCenter, newHalfExtents);
}

bool Physics::Bounding::AABB::intersectsAABB(const AABB &other) const {
    return (minCorner.x <= other.maxCorner.x && maxCorner.x >= other.minCorner.x) &&
           (minCorner.y <= other.maxCorner.y && maxCorner.y >= other.minCorner.y) &&
           (minCorner.z <= other.maxCorner.z && maxCorner.z >= other.minCorner.z);
}

// Tavian Barnes’ branchless slab method
bool Physics::Bounding::AABB::intersectsRay(const glm::vec3 &orig, const glm::vec3 &dir, float &outT) const {
    glm::vec3 invDir = 1.0f / dir;

    glm::vec3 t0s = (minCorner - orig) * invDir;
    glm::vec3 t1s = (maxCorner - orig) * invDir;

    glm::vec3 tsmaller = glm::min(t0s, t1s);
    glm::vec3 tbigger  = glm::max(t0s, t1s);

    float tmin = std::max(std::max(tsmaller.x, tsmaller.y), tsmaller.z);
    float tmax = std::min(std::min(tbigger.x, tbigger.y), tbigger.z);

    outT = tmin;
    return tmax >= std::max(tmin, 0.0f);
}

