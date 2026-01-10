/**
 * @file IPickable.h
 * @brief Interface for objects that support mouse-based selection and interaction.
 */

#pragma once
#include <math/Ray.h>
#include <optional>

/**
 * @interface IPickable
 * @brief Abstract interface for objects that can be selected and interacted with using the mouse.
 *
 * This interface defines the contract for objects that support ray-based picking,
 * hover states, and click handling. It is used by the scene manager to implement
 * interactive object manipulation and selection.
 *
 * The picking system uses CPU-based ray-object intersection tests, with some
 * implementations delegating to GPU compute shaders for complex geometry.
 *
 * @note Implementations must maintain their own hover state and respond to
 *       state changes appropriately (e.g., visual feedback).
 *
 * @see SceneManager::updateHoverState()
 * @see SceneManager::handleMouseButton()
 * @see MathUtils::findFirstHit()
 */
class IPickable {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IPickable() = default;

    /**
     * @brief Tests if a ray intersects this object's geometry.
     *
     * Implementations should test the ray against the object's bounding volume
     * or exact geometry and return the distance to the intersection point if hit.
     *
     * @param ray The picking ray in world space coordinates.
     *
     * @return std::optional<float> containing the distance along the ray to the
     *         intersection point if hit, or std::nullopt if no intersection.
     *
     * @note For performance, implementations should first test against an AABB
     *       before performing exact mesh intersection tests.
     *
     * Example implementation:
     * @code
     * std::optional<float> SceneObject::intersectsRay(const MathUtils::Ray& ray) const {
     *     float aabbDist;
     *     if (!intersectsAABB(ray, aabbDist)) {
     *         return std::nullopt;
     *     }
     *
     *     float meshDist;
     *     if (intersectsMesh(ray, meshDist)) {
     *         return meshDist;
     *     }
     *
     *     return std::nullopt;
     * }
     * @endcode
     *
     * @see SceneObject::intersectsRay()
     * @see Gizmo::intersectsRay()
     * @see MathUtils::intersectsRay()
     */
    virtual std::optional<float> intersectsRay(const Math::Ray& ray) const = 0;

    /**
     * @brief Handles a mouse click on this object.
     *
     * Called by the scene manager when the user clicks on this object.
     * Implementations can respond by:
     * - Changing selection state
     * - Spawning interaction gizmos
     * - Triggering object-specific behaviors
     * - Emitting signals for UI updates
     *
     * @param ray The picking ray that intersected this object.
     * @param distance The distance along the ray to the intersection point.
     *
     * @note This is only called if rayIntersection() returned a valid distance.
     *
     * @see SceneManager::handleMouseButton()
     * @see SceneObject::handleClick()
     * @see Gizmo::handleClick()
     */
    virtual void handleClick(const Math::Ray& ray, float distance) = 0;

    /**
     * @brief Sets the hover state of this object.
     *
     * Called by the scene manager when the mouse cursor enters or leaves this object.
     * Implementations should update their internal state and may trigger visual
     * feedback (e.g., highlighting).
     *
     * The actual visual feedback is typically handled through the rendering
     * system via uniform buffers (isHovered[objectID]).
     *
     * @param hovered true if the mouse is hovering over this object, false otherwise.
     *
     * @see SceneManager::updateHoverState()
     * @see Scene::draw()
     */
    virtual void setHovered(bool hovered) = 0;

    /**
     * @brief Gets the current hover state of this object.
     *
     * @return true if the object is currently being hovered over, false otherwise.
     */
    virtual bool getHovered() const = 0;

    /**
     * @brief Gets the unique identifier for this object.
     *
     * This ID is used to:
     * - Correlate pick results with specific objects
     * - Drive hover/selection highlighting through shader uniforms
     * - Identify objects in the scene hierarchy
     *
     * @return The object's unique identifier.
     *
     * @note This should return the same ID as IDrawable::getObjectID() for
     *       objects implementing a derived IDrawable interface and this interface.
     *
     * @see Scene::allocateObjectID()
     * @see IDrawable::getObjectID()
     */
    virtual uint32_t getObjectID() const = 0;
};