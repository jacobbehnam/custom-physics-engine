#pragma once
#include <QOpenGLFunctions_4_5_Core>
#include <vector>
#include <glm/glm.hpp>
#include <physics/bounding/AABB.h>
#include "graphics/core/InstanceData.h"

/**
 * @brief Vertex data for a single vertex in a mesh.
 *
 * Contains geometric and shading information for rendering.
 */
struct Vertex {
    glm::vec3 pos; ///< Position in local/model space
    glm::vec3 normal; ///< Surface normal

    /**
     * @brief Equality comparison for vertex deduplication.
     * @param other The vertex to compare against
     * @return true if position and normal are identical
     */
    bool operator==(const Vertex& other) const {
        return pos == other.pos &&
               normal == other.normal;
    }
};

/**
 * @brief Hash specialization for Vertex to enable use in unordered containers.
 *
 * Used for efficient vertex deduplication during mesh loading.
 */
namespace std {
    template <>
    struct hash<Vertex> {
        size_t operator()(const Vertex& v) const {
            size_t h1 = hash<float>()(v.pos.x) ^ (hash<float>()(v.pos.y) << 1) ^ (hash<float>()(v.pos.z) << 2);
            size_t h2 = hash<float>()(v.normal.x)   ^ (hash<float>()(v.normal.y)   << 1) ^ (hash<float>()(v.normal.z) << 2);
            return h1 ^ (h2 << 1);
        }
    };
}

/**
 * @brief GPU mesh representation with support for instanced rendering.
 *
 * Manages vertex data, indices, and GPU buffers for a single mesh.
 * Supports both standard and instanced rendering. Meshes are immutable
 * after construction - vertex and index data cannot be modified.
 *
 * @note This class assumes an active OpenGL context during construction.
 * @note The mesh automatically computes an axis-aligned bounding box (AABB)
 *       in local space for collision detection and culling.
 *
 * @see ResourceManager for mesh loading and management
 * @see Rendering::InstanceData for per-instance data layout
 */
class Mesh {
public:
    /**
     * @brief Constructs a mesh from vertex and index data.
     *
     * Uploads data to GPU and sets up vertex attributes for rendering.
     * Automatically computes a local-space AABB for the mesh.
     *
     * @param verts Vertex data (positions, normals)
     * @param idx Index buffer for indexed drawing
     * @param funcs OpenGL function pointers for the current context
     *
     * @note An OpenGL context must be current on this thread
     * @note funcs must not be nullptr
     * @note verts must not be empty
     * @note idx must not be empty and all indices must be valid
     */
    Mesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& idx, QOpenGLFunctions_4_5_Core* funcs);

    /**
     * @brief Destroys the mesh and releases GPU resources.
     *
     * Deletes VAO, VBO, EBO, and instance VBO if they exist.
     */
    ~Mesh();

    /**
     * @brief Draws a single instance of this mesh.
     *
     * Uses the currently bound shader. Model matrix and other uniforms
     * must be set by the caller before drawing.
     *
     * @note A shader must be bound via glUseProgram
     * @note Appropriate uniforms must be set in the bound shader
     */
    void draw() const;

    /**
     * @brief Draws multiple instances of this mesh in a single draw call.
     *
     * More efficient than multiple draw() calls for rendering many copies
     * of the same mesh with different transforms/properties.
     *
     * @param instances Per-instance data (transforms, IDs, colors, etc.)
     *
     * @note A shader must be bound that supports instanced rendering
     * @note instances must not be empty
     * @note Shader must have vertex attributes matching InstanceData layout
     *
     * @see Rendering::InstanceData for the expected data layout
     * @see setupInstanceAttributes() for attribute configuration
     */
    void drawInstanced(const std::vector<Rendering::InstanceData>& instances);

    /**
     * @brief Provides read-only view of vertex data without copying.
     *
     * Returns a non-owning view over the mesh's vertex array.
     *
     * @return Non-owning span view of vertex data
     *
     * @note The returned span is only valid while this Mesh exists
     * @note The span is read-only - use const iteration: `for (const auto& v : span)`
     * @note Cannot modify vertex data through this view (mesh is immutable after construction)
     *
     * @par Example Usage:
     * @code
     * std::span<const Vertex> verts = mesh->getVertices();
     * for (const auto& vertex : verts) {
     *     // Process vertex.pos, vertex.normal, etc.
     * }
     * @endcode
     */
    std::span<const Vertex> getVertices() const;

    /**
     * @brief Provides read-only view of index data without copying.
     *
     * Returns a non-owning view over the mesh's index array.
     *
     * @return Non-owning span view of index data
     *
     * @note The returned span is only valid while this Mesh exists
     * @note The span is read-only - use const iteration
     * @note Cannot modify index data through this view (mesh is immutable after construction)
     *
     * @par Example Usage:
     * @code
     * std::span<const unsigned int> indices = mesh->getIndices();
     * size_t triangleCount = indices.size() / 3;
     * @endcode
     */
    std::span<const unsigned int> getIndices() const;

    /**
     * @brief Gets the local-space axis-aligned bounding box.
     *
     * The AABB is computed during construction and represents the
     * tightest axis-aligned box that contains all vertices.
     *
     * @return Reference to the cached local AABB
     *
     * @note To get a world-space AABB, call getTransformed() on the result
     *       with the object's model matrix
     */
    const Physics::Bounding::AABB& getLocalAABB() const { return localAABB; }
private:
    QOpenGLFunctions_4_5_Core* funcs; ///< OpenGL function pointers
    std::vector<Vertex> vertices; ///< CPU copy of vertex data
    std::vector<unsigned int> indices; ///< CPU copy of index data
    Physics::Bounding::AABB localAABB; ///< Cached local-space bounding box

    unsigned int VAO; ///< Vertex Array Object ID
    unsigned int VBO; ///< Vertex Buffer Object ID
    unsigned int instanceVBO; ///< Instance data VBO ID
    unsigned int EBO; ///< Element/Index Buffer Object ID
    unsigned int indexCount; ///< Number of indices (cached for draw calls)

    /**
     * @brief Configures vertex attributes for instanced rendering.
     *
     * Sets up vertex attribute pointers for per-instance data:
     * - Locations 2-5: Model matrix (4 vec4s)
     * - Location 6: Object ID (uint)
     * - Location 7: Color (vec3)
     *
     * Called during construction to prepare for instanced rendering.
     *
     * @note Vertex divisor is set to 1 for all instance attributes
     * @note Must match the layout expected by shaders using instanced rendering
     */
    void setupInstanceAttributes();

    /**
     * @brief Computes and caches the local-space AABB from vertex data.
     *
     * Finds the minimum and maximum extents across all vertex positions
     * and constructs an AABB that tightly bounds the mesh.
     *
     * Called during construction. The result is cached in localAABB.
     */
    void createLocalAABB();
};

