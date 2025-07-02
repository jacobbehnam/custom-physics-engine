#pragma once
#include <glm/glm.hpp>

class IPickable {
public:
    virtual ~IPickable() = default;
    virtual bool rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance) = 0;
    virtual void handleClick(const glm::vec3& rayOrig, const glm::vec3& rayDir, float distance) = 0;
    virtual void setHovered(bool hovered) = 0;
    virtual bool getHovered() = 0;
};