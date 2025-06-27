#pragma once
#include <glm/glm.hpp>

bool intersectTriangle(
  const glm::vec3& orig,
  const glm::vec3& dir,
  const glm::vec3& v0,
  const glm::vec3& v1,
  const glm::vec3& v2,
  float& outT
);