/**
 * @file ICollider.h
 * @brief Abstract interface for collision detection shapes
 * @ingroup physics
 */

#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include "math/Ray.h"

/**
 * @namespace Physics::Bounding
 * @brief Bounding volume and collision shape definitions
 *
 * Contains interfaces and data structures for geometric collision
 * primitives used by the physics engine. These bounding volumes are
 * responsible for spatial queries such as point containment, closest
 * point queries, and ray intersections.
 *
 * All colliders are defined in local space and are typically transformed
 * to world space before collision tests are performed.
 *
 * @ingroup physics
 */
namespace Physics::Bounding {
    /**
     * @brief Contact point information from collision queries
     *
     * Represents the geometric details of a collision or proximity query.
     * Used for both collision resolution and visualization.
     */
    struct ContactInfo {
        glm::vec3 point; ///< Contact point in world space (closest point on surface)
        glm::vec3 normal; ///< Surface normal at contact point (points outward from surface)
        float penetration; ///< Penetration depth (positive = overlapping, negative = separated)
    };

    /**
     * @interface ICollider
     * @brief Abstract base class for collision volumes in the physics engine
     *
     * Defines the interface for 3D collision primitives such as axis-aligned
     * bounding boxes (AABB) and oriented bounding boxes (OBB). Colliders support
     * both discrete intersection tests and continuous queries for closest points.
     *
     * @note Colliders are defined in local space. Transform them to world space
     *       using getTransformed() before collision checks.
     *
     * @par Ownership Model
     * ICollider instances are typically owned by PhysicsBody objects. When
     * transformed copies are created via getTransformed(), ownership is
     * transferred to the caller.
     *
     * @par Derived Classes
     * - AABB: Axis-aligned bounding box (fast, simple)
     * - BoxCollider: Oriented bounding box (arbitrary rotation)
     * - SphereCollider: (planned, not yet implemented)
     *
     * @see AABB, BoxCollider, PhysicsBody
     */
    class ICollider {
    public:
        /**
         * @brief Virtual destructor for proper cleanup of derived classes
         */
        virtual ~ICollider() = default;

        /**
         * @brief Tests if a point is inside the collider volume
         *
         * @param p Point to test, in the same coordinate space as this collider
         *
         * @return `true` if point is inside or on the surface, `false` otherwise
         *
         * @note For box colliders, "inside" includes the surface (closed volume)
         * @note This test is typically performed in world space after transformation
         */
        virtual bool contains(const glm::vec3& p) const = 0;

        /**
         * @brief Finds the closest point on the collider surface to a given point
         *
         * @param p Query point in the same space as the collider
         * @return ContactInfo containing closest point, normal, and penetration depth
         *
         * @note If p is inside the collider, penetration will be positive
         * @note The normal always points away from the collider surface
         *
         * @par Use Cases
         * - Collision resolution (finding separation vector)
         * - Distance queries
         * - Computing contact manifolds
         *
         * @see ContactInfo
         */
        virtual ContactInfo closestPoint(const glm::vec3& p) const = 0;

        /**
         * @brief Creates a transformed copy of this collider in world space
         *
         * Applies a 4x4 transformation matrix to create a new collider instance
         * representing this shape in world coordinates. This is used to transform
         * local-space colliders to world space for collision testing.
         *
         * @param modelMatrix Transformation matrix (typically from SceneObject)
         *                    Encodes translation, rotation, and scale
         *
         * @return Unique pointer to transformed collider (ownership transferred)
         *
         * @note The transformation extracts:
         *       - Translation: modelMatrix[3].xyz
         *       - Rotation: Extracted via glm::decompose
         *       - Scale: Applied to half-extents/radius
         * @note Caller receives ownership via std::unique_ptr
         * @note Returns nullptr only if transformation fails (rare)
         *
         */
        virtual std::unique_ptr<ICollider> getTransformed(const glm::mat4& modelMatrix) const = 0;

        /**
         * @brief Tests ray-collider intersection
         *
         * @param ray The ray to test against this collider
         * @return Distance along ray if intersection occurs, `std::nullopt` otherwise
         *
         * @note Negative distances may be rejected depending on implementation
         *
         * @see Ray
         */
        virtual std::optional<float> intersectRay(const Math::Ray& ray) const = 0;
    };
}