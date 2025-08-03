#pragma once
#include <glm/glm.hpp>
#include "graphics/interfaces/IPickable.h"
#include <vector>
#include <functional>
#include <algorithm>

struct ObjectSnapshot;

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

  template<typename ScalarS, typename ScalarE>
  ScalarE interpolateCrossing(const ObjectSnapshot &beforeCrossing, const ObjectSnapshot &afterCrossing, ScalarS stopThreshold, std::function<ScalarS(const ObjectSnapshot&)> getStop, std::function<ScalarE(const ObjectSnapshot&)> getExtract) {
    auto sA = getStop(beforeCrossing);
    auto sB = getStop(afterCrossing);
    auto eA = getExtract(beforeCrossing);
    auto eB = getExtract(afterCrossing);

    // avoid division by zero
    if (std::abs(sB - sA) < std::numeric_limits<ScalarS>::epsilon())
      return eB;

    float alpha = (stopThreshold - sA) / (sB - sA);
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    return eA + alpha * (eB - eA);
  }
}
