#include "RotateHandle.h"

#include <iostream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "graphics/core/SceneObject.h"
#include <graphics/core/ResourceManager.h>


RotateHandle::RotateHandle(SceneObject *tgt, Axis ax) : target(tgt), axis(ax) {}

void RotateHandle::onDrag(const Math::Ray& ray) {
    glm::vec3 axisDirection = axisDir(axis);
    glm::vec3& planeNormal = axisDirection;
    glm::vec3 center = target->getPosition();

    // Ray-plane intersection
    float denom = glm::dot(ray.dir, planeNormal);
    if (glm::abs(denom) < 1e-6f) return; // ray is parallel to plane

    float t = glm::dot(center - ray.origin, planeNormal) / denom;
    glm::vec3 currentHitPoint = ray.origin + t * ray.dir;

    glm::vec3 from = glm::normalize(initialHitPoint - center);
    glm::vec3 to   = glm::normalize(currentHitPoint - center);

    float angle = glm::atan(glm::dot(glm::cross(from, to), axisDirection), glm::dot(from, to));

    glm::quat delta = glm::angleAxis(angle, axisDirection);

    glm::quat finalQuat = delta * originalQuat;

    target->setRotationQuat(finalQuat);
}

void RotateHandle::setDragState(glm::vec3 initHitPos) {
    initialHitPoint = initHitPos;
    originalQuat    = target->getRotationQuat();
}

glm::mat4 RotateHandle::getModelMatrix() const {
    glm::mat4 model(1.0f);
    model = glm::translate(model, target->getPosition());
    model = model * rotateFromYToAxis(axis);
    model = glm::scale(model, glm::vec3(scale, scale, scale));
    return model;
}
