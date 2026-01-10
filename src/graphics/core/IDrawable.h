#pragma once
#include <graphics/components/Shader.h>
#include <graphics/components/Mesh.h>

class IDrawable {
public:
    virtual ~IDrawable() = default;

    virtual Shader* getShader() const = 0;
    virtual Mesh* getMesh() const = 0;
    virtual uint32_t getObjectID() const = 0;
};

/**
 * @interface IInstancedDrawable
 * @brief Drawable that supports GPU instanced rendering
 *
 * Objects implementing this interface provide per-instance data
 * and let Scene batch them for efficient rendering.
 */
class IInstancedDrawable : public IDrawable {
public:
    /**
     * @brief Gets the model matrix for this instance
     * @return 4x4 transformation matrix
     */
    virtual glm::mat4 getModelMatrix() const = 0;

    /**
     * @brief Gets additional per-instance data (color, etc.)
     * @return Instance data structure for GPU upload
     */
    virtual InstanceData getInstanceData() const {
        return {getModelMatrix(), getObjectID()};
    }
};

/**
 * @interface ICustomDrawable
 * @brief Drawable with custom rendering logic
 *
 * Objects that need special rendering implement this interface.
 */
class ICustomDrawable : public IDrawable {
public:
    /**
     * @brief Performs custom rendering
     *
     * Implementation is responsible for all GL state management
     * and draw calls.
     */
    virtual void draw() const = 0;
};