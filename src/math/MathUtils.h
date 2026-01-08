/**
* @file MathUtils.h
* @brief Mathematical utility functions for 3D graphics and physics
* @ingroup graphics
*
* Provides ray-object intersection tests, coordinate transformations,
* and geometric utility functions used throughout the rendering and
* physics systems.
*/

#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <optional>
#include "graphics/core/IPickable.h"

struct ObjectSnapshot;

/**
 * @namespace Math
 * @brief Mathematical and geometric utility functions
 *
 * Provides ray intersection tests, coordinate transformations,
 * and interpolation utilities used throughout the rendering and
 * physics systems.
 */
namespace Math {
    /**
    * @brief Tests ray-triangle intersection using Möller-Trumbore algorithm
    *
    * This is a fast, stable algorithm that computes barycentric coordinates
    * and checks if the intersection point lies within the triangle.
    *
    * @param ray Ray to test (origin + direction)
    * @param v0 First triangle vertex
    * @param v1 Second triangle vertex
    * @param v2 Third triangle vertex
    *
    * @return Distance along ray to intersection, or `std::nullopt` if no intersection
    *
    * @note Intersection point = ray.origin + ray.dir * outT
    * @note Ray direction should be normalized for outT to represent actual distance
    *
    * Example:
    * @code
    * Ray ray = {camera.position, rayDir};
    * if (auto t = intersectTriangle(ray, v0, v1, v2)) {
    *     glm::vec3 hitPoint = ray.origin + ray.dir * (*t);
    *     std::cout << "Hit at distance: " << *t << std::endl;
    * }
    * @endcode
    * @see https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
    */
    inline std::optional<float> intersectTriangle(
        const Ray& ray,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2
    ) {
        constexpr float EPSILON = std::numeric_limits<float>::epsilon();

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        glm::vec3 pvec = glm::cross(ray.dir, edge2);
        float det = glm::dot(edge1, pvec);

        // If the determinant is near zero, ray lies in plane of triangle or is backfacing
        if (det > -EPSILON && det < EPSILON) return std::nullopt;

        float invDet = 1.0f / det;

        glm::vec3 tvec = ray.origin - v0;
        float u = glm::dot(tvec, pvec) * invDet;
        if (u < 0.0f || u > 1.0f) return std::nullopt;

        glm::vec3 qvec = glm::cross(tvec, edge1);
        float v = glm::dot(ray.dir, qvec) * invDet;
        if (v < 0.0f || u + v > 1.0f) return std::nullopt;

        // compute t to find out where the intersection point is on the line
        float t = glm::dot(edge2, qvec) * invDet;
        if (t < EPSILON) return std::nullopt;  // intersection is behind the ray origin

        return t;
    }

    /**
     * @brief Converts screen coordinates to a world-space ray direction
     *
     * Transforms 2D mouse/screen coordinates into a 3D ray emanating from
     * the camera position. This is essential for mouse picking and interaction.
     *
     * @param mouseX Screen X coordinate in pixels (0 = left edge)
     * @param mouseY Screen Y coordinate in pixels (0 = top edge)
     * @param fbWidth Framebuffer width in pixels
     * @param fbHeight Framebuffer height in pixels
     * @param view Camera view matrix (world-to-camera transform)
     * @param projection Camera projection matrix (camera-to-clip transform)
     *
     * @return Normalized direction vector in world space
     *
     * @note The returned direction is normalized and ready to use with `Ray::dir`
     * @note Mouse Y is typically inverted in screen space (0 = top)
     *
     * Example usage:
     * @code
     * glm::vec3 rayDir = screenToWorldRayDirection(
     *     mouseX, mouseY,
     *     windowWidth, windowHeight,
     *     camera.getViewMatrix(),
     *     camera.getProjMatrix()
     * );
     * Ray ray = {camera.position, rayDir};
     * @endcode
     */
    inline glm::vec3 screenToWorldRayDirection(
        double mouseX,
        double mouseY,
        int fbWidth,
        int fbHeight,
        const glm::mat4 &view,
        const glm::mat4 &projection
    ) {
        float x = (2.0f * mouseX) / fbWidth - 1.0f;
        float y = 1.0f - (2.0f * mouseY) / fbHeight;

        glm::vec4 clip = glm::vec4(x, y, -1.0f, 1.0f);
        glm::mat4 invProj = glm::inverse(projection);
        glm::vec4 eye = invProj * clip;
        eye.z = -1.0f;
        eye.w = 0.0f;

        glm::mat4 invView = glm::inverse(view);
        glm::vec4 worldDir4 = invView * eye;

        return glm::normalize(glm::vec3(worldDir4));
    }

    /**
     * @brief Represents the result of a ray-object intersection test.
     *
     * Contains information about which object was hit by a ray and the distance
     * from the ray origin to the intersection point.
     */
    struct HitResult {
        IPickable* object; ///< Pointer to the hit object, or nullptr if no intersection.
        float distance; ///< Distance from the ray origin to the intersection point. Infinity if no hit.
    };

    /**
     * @brief Finds the first object intersected by a ray.
     *
     * Performs ray intersection tests against all objects in the provided vector and
     * returns the closest hit as a `HitResult`, which contains both the object
     * and the distance to the intersection. An optional priority object can be
     * provided; if it is intersected by the ray, it will be returned immediately
     * without testing the remaining objects.
     *
     * @param objects List of pickable objects to test against.
     * @param ray Ray to test (origin and direction).
     * @param priority Optional object to prioritize; if hit, it takes precedence.
     *
     * @return `std::optional<HitResult>`, or `std::nullopt` if no object is hit.:
     *
     * @note If the priority object is hit, the function returns immediately.
     * @note Objects are tested in list order; consider spatial acceleration structures for large scenes.
     * @note The function is O(n) in the number of objects.
     * @note Not thread-safe if the `objects` list is modified concurrently.
     *
     * @see HitResult
     */
    inline std::optional<HitResult> findFirstHit(
        const std::vector<IPickable*>& objects,
        const Ray& ray,
        IPickable* priority = nullptr
    ) {
        float distance = std::numeric_limits<float>::infinity();
        IPickable* best = nullptr;

        for (IPickable* obj : objects) {
            float t;
            if (!obj->rayIntersection(ray.origin, ray.dir, t))
                continue;

            // If this is the priority object, take it immediately
            if (obj == priority) {
                distance = t;
                return HitResult{obj, distance};
            }

            if (t < distance) {
                distance = t;
                best = obj;
            }
        }

        return HitResult{best, distance};
    }
}
