/**
 * @file IDrawable.h
 * @brief Core interfaces for the rendering system supporting both instanced and custom drawing.
 */

#pragma once
#include <graphics/components/Shader.h>
#include <graphics/components/Mesh.h>

/**
 * @interface IDrawable
 * @brief Base interface for all renderable objects in the scene.
 *
 * IDrawable defines the minimal contract for objects that can be rendered.
 * It provides access to the core rendering resources (shader, mesh, object ID)
 * without specifying how rendering is performed.
 *
 * Derived interfaces (IInstancedDrawable, ICustomDrawable) define specific
 * rendering strategies:
 * - IInstancedDrawable: Objects rendered via GPU instancing (batched)
 * - ICustomDrawable: Objects with specialized rendering logic
 *
 * This design allows Scene::draw() to efficiently batch compatible objects
 * while supporting specialized rendering when needed.
 *
 * @note Objects must implement either IInstancedDrawable or ICustomDrawable,
 *       not IDrawable directly.
 *
 * @see IInstancedDrawable
 * @see ICustomDrawable
 * @see Scene::draw()
 */
class IDrawable {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IDrawable() = default;

    /**
     * @brief Gets the shader used to render this object.
     *
     * The shader is used as part of the batch key for instanced rendering,
     * grouping objects that share the same shader and mesh.
     *
     * @return Pointer to the Shader object. Must not be null.
     *
     * @see ResourceManager::getShader()
     * @see Scene::draw()
     */
    virtual Shader* getShader() const = 0;

    /**
     * @brief Gets the mesh geometry for this object.
     *
     * The mesh is used as part of the batch key for instanced rendering,
     * grouping objects that share the same shader and mesh.
     *
     * @return Pointer to the Mesh object. Must not be null.
     *
     * @see ResourceManager::getMesh()
     * @see Scene::draw()
     */
    virtual Mesh* getMesh() const = 0;

    /**
     * @brief Gets the unique identifier for this object.
     *
     * This ID is used for:
     * - Object picking and selection
     * - Hover state highlighting via shader uniforms
     * - Per-instance identification in the rendering pipeline
     *
     * @return The object's unique identifier.
     *
     * @see Scene::allocateObjectID()
     * @see Scene::freeObjectID()
     */
    virtual uint32_t getObjectID() const = 0;
};


/**
 * @interface IInstancedDrawable
 * @brief Interface for objects that support GPU instanced rendering.
 *
 * Objects implementing this interface can be efficiently batched and rendered
 * together when they share the same mesh and shader. The Scene groups compatible
 * objects and renders them in a single instanced draw call, significantly
 * reducing CPU overhead and draw call count.
 *
 * This is the preferred rendering path for most scene objects (SceneObject, etc.)
 * as it provides optimal performance for large numbers of similar objects.
 *
 * @note The default getInstanceData() implementation provides basic transform
 *       and ID data. Override for custom per-instance attributes (e.g., color).
 *
 * Example usage:
 * @code
 * class SceneObject : public IInstancedDrawable {
 *     glm::mat4 getModelMatrix() const override {
 *         return computeTransformMatrix();
 *     }
 * };
 * @endcode
 *
 * @see Scene::draw()
 * @see Mesh::drawInstanced()
 * @see Rendering::InstanceData
 */
class IInstancedDrawable : public IDrawable {
public:
    /**
     * @brief Gets the model transformation matrix for this instance.
     *
     * The model matrix transforms vertices from model space to world space,
     * encoding the object's position, rotation, and scale. This matrix is
     * uploaded to the GPU as part of the instance data.
     *
     * @return The 4x4 model transformation matrix.
     *
     * @note This is called once per frame during batch preparation.
     */
    virtual glm::mat4 getModelMatrix() const = 0;

    /**
     * @brief Gets the complete per-instance data for GPU upload.
     *
     * Constructs an InstanceData structure containing the model matrix,
     * object ID, and any additional per-instance attributes. This data
     * is uploaded to the GPU's instance buffer for instanced rendering.
     *
     * The default implementation provides model matrix and object ID.
     * Override to add custom attributes (e.g., per-instance color).
     *
     * @return InstanceData structure for this object.
     *
     * @note The InstanceData layout must match the vertex attribute setup
     *       in Mesh::setupInstanceAttributes().
     *
     * @see Rendering::InstanceData
     * @see Mesh::drawInstanced()
     */
    virtual Rendering::InstanceData getInstanceData() const {
        return {getModelMatrix(), getObjectID()};
    }
};

/**
 * @interface ICustomDrawable
 * @brief Interface for objects with specialized rendering requirements.
 *
 * Objects implementing this interface have full control over their rendering
 * process. This is used for objects that:
 * - Cannot be efficiently batched (e.g., Gizmos with dynamic geometry)
 * - Require special rendering state (e.g., depth testing disabled)
 * - Need custom draw logic (e.g., multiple passes, compute shaders)
 *
 * Custom drawables bypass the instancing system and are rendered individually
 * after all instanced objects.
 *
 * @warning Custom rendering has higher overhead than instanced rendering.
 *          Use IInstancedDrawable when possible.
 *
 * Example usage:
 * @code
 * class Gizmo : public ICustomDrawable {
 *     void draw() const override {
 *         glDisable(GL_DEPTH_TEST);
 *         shader->use();
 *         mesh->drawInstanced(handleInstances);
 *         glEnable(GL_DEPTH_TEST);
 *     }
 * };
 * @endcode
 *
 * @see Scene::draw()
 * @see Gizmo
 */
class ICustomDrawable : public IDrawable {
public:
    /**
     * @brief Performs custom rendering for this object.
     *
     * Implementations are responsible for:
     * - Binding appropriate shaders and setting uniforms
     * - Managing OpenGL state (depth testing, blending, etc.)
     * - Issuing draw calls (direct or instanced)
     * - Restoring OpenGL state if modified
     *
     * This method is called after all instanced objects are rendered.
     *
     * @note The camera matrices uniform buffer is already bound when called.
     *
     * @see Scene::draw()
     */
    virtual void draw() const = 0;
};