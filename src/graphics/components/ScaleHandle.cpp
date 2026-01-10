#include "ScaleHandle.h"
#include <graphics/core/ResourceManager.h>
#include <iostream>

ScaleHandle::ScaleHandle(SceneObject *tgt, Axis ax) : target(tgt), axis(ax) {}

void ScaleHandle::onDrag(const Math::Ray& ray) {
    glm::vec3 axisDirection = axisDir(axis);
    glm::vec3 localAxisDirection = target->getRotationQuat() * axisDirection;
    // To solve for t, minimize the quantity || (rayOrig + rayDir * t) - initialHitPoint ||
    float t = glm::dot(-(ray.origin - initialHitPoint), ray.dir);

    glm::vec3 delta = (ray.origin + ray.dir * t) - initialHitPoint;

    float scaleAmount = glm::dot(delta, localAxisDirection);
    target->setScale(originalScale + axisDirection * scaleAmount);
}

void ScaleHandle::setDragState(glm::vec3 initHitPos) {
    initialHitPoint = initHitPos;
    originalScale = target->getScale();
}

glm::mat4 ScaleHandle::getModelMatrix() const {
    glm::mat4 model(1.0f);
    model = glm::translate(model, target->getPosition());
    model = model * glm::mat4_cast(target->getRotationQuat());
    model = model * rotateFromYToAxis(axis);
    model = glm::scale(model, glm::vec3(thickness, length, thickness));
    return model;
}
