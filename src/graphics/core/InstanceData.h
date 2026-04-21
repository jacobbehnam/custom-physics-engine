#pragma once
#include <glm/glm.hpp>

namespace Rendering {
    /**
     * @brief Per-instance data for instanced rendering.
     *
     * This struct contains all data that varies per instance in an instanced draw call.
     * Fields are laid out to match shader vertex attribute expectations.
     *
     * @note When adding new fields:
     * - Add them at the end to maintain backward compatibility
     * - Update setupInstanceAttributes() in Mesh.cpp
     * - Update shader attribute locations accordingly
     * - Keep alignment in mind (vec4/mat4 alignment is important)
     */
    struct InstanceData {
        // Transform (locations 2-5)
        glm::mat4 model;

        // Object identification (location 6)
        uint32_t objectID;

        // Visual properties (location 7)
        glm::vec3 color;

        // Future fields can be added here:

        // Default constructor
        InstanceData() : model(1.0f), objectID(0), color(1.0f, 1.0f, 1.0f) {}

        // Convenience constructor for common use case
        InstanceData(const glm::mat4& m, uint32_t id, const glm::vec3& col = glm::vec3(1.0f))
            : model(m), objectID(id), color(col) {}
    };
}
