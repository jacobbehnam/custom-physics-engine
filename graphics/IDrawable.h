#pragma once
#include <glm/vec3.hpp>
#include <graphics/Shader.h>

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};

class IDrawable {
public:
    virtual ~IDrawable() = default;
    virtual void draw() const = 0;
    virtual Shader* getShader() const = 0;

    // For now every drawable object is clickable (will change later)
    virtual bool rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance) const = 0;
    virtual void handleClick(const glm::vec3& rayOrig, const glm::vec3& rayDir, float distance) = 0;
};