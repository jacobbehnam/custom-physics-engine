#pragma once
#include <glm/vec3.hpp>

namespace Math {
    /**
    * @brief Represents a ray in 3D space
    *
    * Used for mouse picking, collision detection, and line-of-sight tests.
    * The ray is defined parametrically as: \f$P(t) = \texttt{origin} + t \cdot \texttt{dir}\f$
    */
    struct Ray {
        glm::vec3 origin; ///< Starting point of the ray in world space
        glm::vec3 dir; ///< Direction vector (should be normalized)
    };
}