#pragma once
#include <glm/glm.hpp>
#include "graphics/interfaces/IPickable.h"
#include <vector>

namespace MathUtils {
  struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
  };

  bool intersectTriangle(
    const glm::vec3& orig,
    const glm::vec3& dir,
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2,
    float& outT);

  glm::vec3 screenToWorldRayDirection(
    double mouseX,
    double mouseY,
    int fbWidth,
    int fbHeight,
    const glm::mat4& view,
    const glm::mat4& projection);

  IPickable* findFirstHit(const std::vector<IPickable*>& objects, const Ray& ray, float &outT, IPickable* priority = nullptr);
}