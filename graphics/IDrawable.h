#pragma once
#include <glm/vec3.hpp>

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};

class IDrawable {
public:
    virtual ~IDrawable() = default;
    virtual void draw() const = 0;
};