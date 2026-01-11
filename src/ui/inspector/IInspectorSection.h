/**
 * @file IInspectorSection.h
 * @brief Interface for inspector panel sections in the editor UI.
 */

#pragma once
#include <QWidget>

class SceneObject;

/**
 * @interface IInspectorSection
 * @brief Abstract base class for inspector panel sections.
 *
 * IInspectorSection defines the contract for UI panels that display and edit
 * properties of selected scene objects. Each section is responsible for a
 * specific aspect of an object (transform, physics, forces, etc.).
 *
 * Architecture:
 * - Inherits from QWidget to integrate with Qt's UI system
 * - Manages its own UI components (labels, spinboxes, checkboxes, etc.)
 * - Binds to a scene object's properties via getter/setter lambdas
 * - Updates automatically via periodic refresh() calls
 *
 * Special Cases:
 * - **Object-specific sections** (Transform, Physics, Forces):
 *   Show/hide based on object selection and applicability
 * - **Global sections** (Globals):
 *   May display settings independent of selection, implementing inverse
 *   visibility (visible when no object selected). These sections may ignore
 *   the object parameter in load() and remain visible during unload().
 *
 * @note Sections should handle nullptr gracefully in refresh() in case
 *       the object is deleted while loaded.
 *
 * @see InspectorWidget
 * @see TransformInspectorWidget
 * @see PhysicsInspectorWidget
 * @see GlobalsInspectorWidget
 * @see ForcesInspectorWidget
 * @see InspectorRow
 */
class IInspectorSection : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IInspectorSection() = default;

    /**
     * @brief Loads and binds this section to a scene object.
     *
     * Called when an object is selected in the scene. Implementations should:
     * - Store the object pointer for property access
     * - Show the section if applicable to this object type
     * - Initialize UI widgets with current object values via refresh()
     *
     * The object pointer is guaranteed to be non-null. For clearing the
     * selection, use unload() instead.
     *
     * @param object The scene object to inspect. Must not be nullptr.
     *
     * @note If the section doesn't apply to this object type (e.g., physics
     *       section for non-physics objects), the implementation should hide
     *       itself via setVisible(false).
     *
     * @see unload()
     * @see refresh()
     * @see InspectorWidget::loadObject()
     */
    virtual void load(SceneObject* object) = 0;

    /**
     * @brief Unloads the current object and clears section state.
     *
     * Called when:
     * - Selection is cleared (no object selected)
     * - Before loading a new object
     * - The inspected object is about to be deleted
     *
     * Implementations should:
     * - Clear the stored object pointer (set to nullptr)
     * - Hide the section via setVisible(false)
     * - Clean up any object-specific state
     *
     * After unload(), the section should be in a safe state where refresh()
     * can be called without side effects (though it typically does nothing).
     *
     * @see load()
     * @see InspectorWidget::unloadObject()
     */
    virtual void unload() = 0;

    /**
     * @brief Updates UI widgets from the current object's state.
     *
     * Called periodically (typically every 100ms via QTimer) to keep the UI
     * synchronized with the object's properties, which may change due to:
     * - Physics simulation updates
     * - Interactive gizmo manipulation
     * - Numerical solver operations
     * - Direct property modifications from code
     *
     * Implementations should:
     * - Check if an object is loaded (guard against nullptr)
     * - Read current property values from the object
     * - Update UI widgets using QSignalBlocker to prevent feedback loops
     *
     * This is a read-only operation - it should not modify the object,
     * only reflect its current state in the UI.
     *
     * @note Must be safe to call even if no object is loaded (should no-op).
     * @note Must use QSignalBlocker when updating widgets to prevent
     *       triggering change handlers that would write back to the object.
     *
     * Example:
     * @code
     * void MySection::refresh() {
     *     if (!selectedObject) return; // Safe no-op when unloaded
     *
     *     for (auto& row : rows) {
     *         row.refresh(); // InspectorRow handles signal blocking
     *     }
     * }
     * @endcode
     *
     * @see load()
     * @see InspectorRow::refresh()
     * @see InspectorWidget::refresh()
     */
    virtual void refresh() = 0;

protected:
    /**
     * @brief Protected constructor enforces use as base class only.
     *
     * @param parent Parent widget for Qt's object tree and memory management.
     *               Typically the InspectorWidget that owns this section.
     */
    explicit IInspectorSection(QWidget* parent = nullptr) : QWidget(parent) {}
};