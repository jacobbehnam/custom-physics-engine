#include "MathUtils.h"

namespace MathUtils {
    // Helper function that implements the Moller–Trumbore ray/triangle intersection algorithm
    bool intersectTriangle(
        const glm::vec3& orig,
        const glm::vec3& dir,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        float& outT
    ) {
        const float EPSILON = 1e-8f;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        glm::vec3 pvec = glm::cross(dir, edge2);
        float det = glm::dot(edge1, pvec);

        // If the determinant is near zero, ray lies in plane of triangle or is backfacing
        if (std::abs(det) < EPSILON) return false;

        float invDet = 1.0f / det;

        glm::vec3 tvec = orig - v0;
        float u = glm::dot(tvec, pvec) * invDet;
        if (u < 0.0f || u > 1.0f) return false;

        glm::vec3 qvec = glm::cross(tvec, edge1);
        float v = glm::dot(dir, qvec) * invDet;
        if (v < 0.0f || u + v > 1.0f) return false;

        // compute t to find out where the intersection point is on the line
        float t = glm::dot(edge2, qvec) * invDet;
        if (t < EPSILON) return false;  // intersection is behind the ray origin

        outT = t;
        return true;
    }

    glm::vec3 screenToWorldRayDirection(double mouseX, double mouseY, int fbWidth, int fbHeight, const glm::mat4 &view, const glm::mat4 &projection) {
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
}
