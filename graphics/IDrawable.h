#pragma once
#include <glm/vec3.hpp>
#include <graphics/Shader.h>

class IDrawable {
public:
    virtual ~IDrawable() = default;
    virtual void draw() const = 0;
    virtual Shader* getShader() const = 0;
};