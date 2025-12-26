#pragma once
#include <glm/vec3.hpp>
#include <graphics/components/Shader.h>
#include <graphics/components/Mesh.h>

class IDrawable {
public:
    virtual ~IDrawable() = default;

    virtual void draw() const = 0;
    virtual Shader* getShader() const = 0;
    virtual Mesh* getMesh() const = 0;
    virtual uint32_t getObjectID() const = 0;

    virtual glm::mat4 getModelMatrix() const = 0;
};