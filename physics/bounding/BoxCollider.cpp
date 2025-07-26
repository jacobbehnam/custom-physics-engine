#include "BoxCollider.h"

Physics::Bounding::BoxCollider::BoxCollider(const glm::vec3 &center, const glm::vec3 &halfExtents, const glm::quat &rotation)
    : center(center), halfExtents(halfExtents), rotation(rotation) {}

bool Physics::Bounding::BoxCollider::contains(const glm::vec3 &p) const {
    glm::vec3 local = glm::inverse(rotation) * (p - center);
    return glm::all(glm::lessThanEqual(glm::abs(local), halfExtents));
}

ContactInfo Physics::Bounding::BoxCollider::closestPoint(const glm::vec3 &p) const {
    glm::vec3 local = glm::inverse(rotation) * (p - center);
    glm::vec3 clamped = glm::clamp(local, -halfExtents, halfExtents);
    glm::vec3 worldPoint = rotation * clamped + center;

    glm::vec3 delta = local - clamped;
    float outsideDist = glm::length(delta);

    glm::vec3 localNormal;
    float penetration;
    if (outsideDist > 0.0f) {
        // Point is outside: normal points from box toward point
        localNormal = glm::normalize(delta);
        penetration = -outsideDist;
    } else {
        glm::vec3 d = halfExtents - glm::abs(local);
        if (d.x <= d.y && d.x <= d.z) {
            localNormal = glm::vec3((local.x > 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f);
            penetration = d.x;
        } else if (d.y <= d.x && d.y <= d.z) {
            localNormal = glm::vec3(0.0f, (local.y > 0.0f) ? 1.0f : -1.0f, 0.0f);
            penetration = d.y;
        } else {
            localNormal = glm::vec3(0.0f, 0.0f, (local.z > 0.0f) ? 1.0f : -1.0f);
            penetration = d.z;
        }
    }
    glm::vec3 worldNormal = rotation * localNormal;

    return { worldPoint, worldNormal, penetration };
}

bool Physics::Bounding::BoxCollider::intersectRay(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float &outT) const {
    glm::vec3 localOrig = glm::inverse(rotation) * (rayOrig - center);
    glm::vec3 localDir  = glm::inverse(rotation) * rayDir;

    glm::vec3 invDir = 1.0f / localDir;
    glm::vec3 tMinVec = (-halfExtents - localOrig) * invDir;
    glm::vec3 tMaxVec = ( halfExtents - localOrig) * invDir;

    glm::vec3 tsmaller = glm::min(tMinVec, tMaxVec);
    glm::vec3 tbigger = glm::max(tMinVec, tMaxVec);

    float tmin = std::max(std::max(tsmaller.x, tsmaller.y), tsmaller.z);
    float tmax = std::min(std::min(tbigger.x, tbigger.y), tbigger.z);

    // no intersection or box is behind
    if (tmin > tmax || tmax < 0.0f)
        return false;

    outT = tmin;
    return tmax >= std::max(tmin, 0.0f);
}
