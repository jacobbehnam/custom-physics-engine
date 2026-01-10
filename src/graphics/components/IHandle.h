/**
 * @file IHandle.h
 * @brief Interface for interactive manipulation handles on gizmos.
 */

#pragma once
#include "../core/IDrawable.h"
#include <glm/gtc/quaternion.hpp>

/**
 * @enum Axis
 * @brief Cardinal axes for handle orientation.
 */
enum class Axis {
    X, Y, Z
};

/**
 * @brief Converts an axis enum to its corresponding unit vector.
 *
 * @param a The axis to convert.
 * @return Unit vector along the specified axis.
 */
constexpr glm::vec3 axisDir(Axis a) {
    switch (a) {
        case Axis::X:
            return {1,0,0};
        case Axis::Y:
            return {0,1,0};
        default:
            return {0,0,1};
    }
}

class Mesh;

/**
 * @interface IHandle
 * @brief Abstract interface for gizmo manipulation handles.
 *
 * IHandle defines the behavior contract for interactive manipulation tools
 * (translation, rotation, scale handles) without dictating how they are rendered.
 *
 * Key design decisions:
 * - Does NOT inherit from IDrawable - rendering is managed by the owning Gizmo
 * - Handles are purely behavioral - they respond to drag operations
 * - Each handle operates along/around a specific axis
 *
 * @see Gizmo
 * @see TranslateHandle
 * @see RotateHandle
 * @see ScaleHandle
 */
class IHandle{
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IHandle() = default;

    /**
     * @brief Handles continuous dragging motion.
     *
     * Called every frame while the handle is being dragged. Implementations
     * should:
     * - Project the ray onto the handle's constraint space (axis/plane)
     * - Compute the transformation delta since drag started
     * - Apply the transformation to the target object
     *
     * @param ray The current mouse ray in world space.
     *
     * @note This is called after setDragState() has initialized the drag.
     *
     * @see setDragState()
     */
    virtual void onDrag(const Math::Ray& ray) = 0;

    /**
     * @brief Initializes the drag operation state.
     *
     * Called once when the user begins dragging this handle. Implementations
     * should capture:
     * - The initial hit position on the handle
     * - The target object's current transformation
     * - Any other state needed to compute relative deltas during onDrag()
     *
     * @param initHitPos The world-space position where the ray first hit the handle.
     *
     * @see onDrag()
     */
    virtual void setDragState(glm::vec3 initHitPos) = 0;

    /**
     * @brief Gets the axis direction this handle operates along/around.
     *
     * @return Unit vector representing the handle's axis direction.
     *
     * @note This is used for rendering (color coding) and for identifying
     *       which handle the user is interacting with.
     */
    virtual glm::vec3 getAxisDir() const = 0;

    /**
     * @brief Gets the model transformation matrix for rendering this handle.
     *
     * The model matrix encodes:
     * - Position: Target object's position
     * - Rotation: Aligned to the handle's axis
     * - Scale: Handle's visual size
     *
     * @return The 4x4 model transformation matrix.
     *
     * @note This is used by Gizmo for instanced rendering of all handles.
     */
    virtual glm::mat4 getModelMatrix() const = 0;

protected:
    /**
     * @brief Protected constructor to enforce interface-only usage.
     */
    IHandle() = default;

    /**
     * @brief Computes rotation matrix to align mesh from +Y to target axis.
     *
     * Handle meshes are authored pointing along +Y. This helper rotates them
     * to point along X, Y, or Z as needed.
     *
     * @param axis The target axis to align to.
     * @return Rotation matrix transforming +Y to the specified axis.
     *
     * @note Returns identity if axis is already +Y (within tolerance).
     */
    static glm::mat4 rotateFromYToAxis(Axis axis) {
        glm::vec3 from = {0, 1, 0};
        glm::vec3 to = axisDir(axis);
        glm::vec3 crossA = glm::cross(from, to);
        float cosA = glm::dot(from, to);

        if (glm::length(crossA) < 1e-3f) {
            return glm::mat4(1.0f);
        }

        float angle = std::acos(glm::clamp(cosA, -1.0f, 1.0f));
        return glm::rotate(glm::mat4(1.0f), angle, glm::normalize(crossA));
    }
};
