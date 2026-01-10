/**
 * @file Gizmo.h
 * @brief Interactive 3D manipulation gizmo for scene objects.
 */

#pragma once
#include <graphics/components/Shader.h>
#include <graphics/core/Camera.h>
#include "IHandle.h"

#include "../core/IPickable.h"

class SceneManager;
class SceneObject;
class Scene;

/**
 * @enum GizmoType
 * @brief Types of transformation gizmos.
 */
enum class GizmoType {
    TRANSLATE,
    ROTATE,
    SCALE
};

/**
 * @class Gizmo
 * @brief Interactive 3D manipulation widget for transforming scene objects.
 *
 * A Gizmo is a visual overlay that appears on selected objects, providing
 * intuitive handles for translation, rotation, or scaling. It manages
 * multiple IHandle instances (one per axis) and coordinates their rendering
 * and interaction.
 *
 * Architecture:
 * - Gizmo owns and renders all handles as a batch
 * - Implements ICustomDrawable for special rendering (depth-test disabled)
 * - Implements IPickable for ray-based handle selection
 * - Handles perform the actual transformation math
 *
 * Rendering approach:
 * - All handles share the same mesh (arrow/circle/cube)
 * - Instanced rendering with per-handle transforms and colors
 * - Rendered without depth testing to stay visible over scene
 * - Hover highlighting shows which handle the cursor is over
 *
 * Interaction flow:
 * 1. User hovers -> rayIntersection() identifies handle -> setHovered(true)
 * 2. User clicks -> handleClick() activates handle, calls setDragState()
 * 3. User drags -> handleDrag() updates via handle's onDrag()
 * 4. User releases -> handleRelease() deactivates handle
 *
 * @note Only one handle can be active (dragging) at a time.
 *
 * @see IHandle
 * @see TranslateHandle
 * @see RotateHandle
 * @see ScaleHandle
 * @see SceneManager::setGizmoFor()
 */
class Gizmo : public ICustomDrawable, public IPickable{
public:
    /**
     * @brief Constructs a gizmo for the specified target object.
     *
     * @param type The type of gizmo (translate/rotate/scale).
     * @param sceneManager Pointer to the scene manager (for object ID allocation).
     * @param target The scene object this gizmo will manipulate.
     *
     * @note Allocates object IDs for the gizmo and all its handles.
     */
    Gizmo(GizmoType type, SceneManager* sceneManager, SceneObject* target);

    /**
     * @brief Destructor - frees object IDs and cleans up handles.
     */
    ~Gizmo();

    // ==================== ICustomDrawable ====================
    /**
     * @brief Renders all handles with custom depth-test-disabled rendering.
     *
     * Disables depth testing so gizmo stays visible through other objects,
     * then uses instanced rendering to draw all handles in one call.
     *
     * Each handle is colored based on its axis (R/G/B for X/Y/Z).
     * The active handle (if dragging) uses its own object ID for precise
     * picking, otherwise all handles share the gizmo's object ID.
     */
    void draw() const override;

    /**
     * @brief Gets the gizmo's unique identifier.
     * @return The gizmo's object ID.
     */
    uint32_t getObjectID() const override;

    /**
     * @brief Gets the shared mesh used for all handles.
     * @return Pointer to the handle mesh (arrow/circle/cube).
     */
    Mesh* getMesh() const override { return handleMesh; }

    /**
     * @brief Gets the shader used for gizmo rendering.
     * @return Pointer to the gizmo shader.
     */
    Shader* getShader() const override;

    // ==================== IPickable ====================
    /**
     * @brief Tests if a ray intersects any of the gizmo's handles.
     *
     * Tests each handle's AABB for intersection and returns the closest hit.
     * Updates hoveredHandle to support hover highlighting.
     *
     * @param ray The picking ray in world space.
     * @return Distance to the closest handle hit, or std::nullopt if no hit.
     *
     * @note Sets hoveredHandle as a side effect (hence mutable).
     */
    std::optional<float> intersectsRay(const Math::Ray& ray) const override;

    /**
     * @brief Handles mouse click on a gizmo handle.
     *
     * Activates the hovered handle and initializes its drag state.
     *
     * @param ray The ray that hit the handle.
     * @param distance Distance along the ray to the hit point.
     *
     * @note Only called if rayIntersection() returned a valid distance.
     */
    void handleClick(const Math::Ray& ray, float distance) override;

    /**
     * @brief Sets the gizmo's hover state.
     *
     * @param hovered true if mouse is over any handle, false otherwise.
     */
    void setHovered(bool hovered) override;

    /**
     * @brief Gets the gizmo's hover state.
     * @return true if hovered, false otherwise.
     */
    bool getHovered() const override;

    // ==================== Gizmo-Specific API ====================
    /**
     * @brief Handles mouse release - deactivates the active handle.
     *
     * Clears activeHandle and sets isDragging to false.
     */
    void handleRelease();

    /**
     * @brief Updates the active handle during a drag operation.
     *
     * Calls onDrag() on the active handle with the current mouse ray.
     *
     * @param ray The current mouse ray in world space.
     */
    void handleDrag(const Math::Ray& ray);

    /**
     * @brief Gets the object this gizmo is manipulating.
     * @return Pointer to the target scene object.
     */
    SceneObject* getTarget() const { return target; }

    /**
     * @brief Gets the currently active (being dragged) handle.
     * @return Pointer to the active handle, or nullptr if none.
     */
    IHandle* getActiveHandle() const { return activeHandle; }

    /**
     * @brief Checks if a drag operation is in progress.
     * @return true if currently dragging a handle, false otherwise.
     */
    bool getIsDragging() const { return isDragging; }

private:
    Scene* ownerScene; ///< Scene for ID allocation
    SceneObject* target; ///< Object being manipulated
    std::vector<std::unique_ptr<IHandle>> handles; ///< Owned handles (3 typically, one per axis)

    mutable IHandle* hoveredHandle = nullptr; ///< Handle under cursor (mutable for rayIntersection const)
    IHandle* activeHandle = nullptr; ///< Handle being dragged

    Shader* shader; ///< Gizmo rendering shader
    Mesh* handleMesh = nullptr; ///< Shared mesh for all handles

    uint32_t objectID; ///< Gizmo's object ID
    bool isDragging = false; ///< true during active drag
    bool isHovered = false; ///< true when any handle is hovered
};