#pragma once
#include <glm/glm.hpp>

struct ContactInfo {
    glm::vec3 point;
    glm::vec3 normal;
    float penetration;
};

class ICollider {
public:
    virtual ~ICollider() = default;

    virtual bool contains(const glm::vec3& p) const = 0;
    virtual ContactInfo closestPoint(const glm::vec3& p) const = 0;

    virtual bool intersectRay(const glm::vec3& rayOrig, const glm::vec3& rayDir, float& outT) const = 0;
};