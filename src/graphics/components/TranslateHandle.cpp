#include "TranslateHandle.h"

#include <iostream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "graphics/core/SceneObject.h"

TranslateHandle::TranslateHandle(SceneObject *tgt, Axis ax, uint32_t objID) : target(tgt), axis(ax), objectID(objID) {}

glm::mat4 TranslateHandle::getModelMatrix() const {
    glm::mat4 model(1.0f);
    model = glm::translate(model, target->getPosition());
    model = model * rotateFromYToAxis(axis);
    model = glm::scale(model, glm::vec3(thickness, length, thickness));
    return model;
}

void TranslateHandle::onDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir) {
    glm::vec3 axisDirection = axisDir(axis);
    // To solve for t, minimize the quantity || (rayOrig + rayDir * t) - initialHitPoint ||
    float t = glm::dot(-(rayOrig - initialHitPoint), rayDir);

    glm::vec3 delta = (rayOrig + rayDir * t) - initialHitPoint;
    // apply translation only along the axis direction component:
    float moveAmount = glm::dot(delta, axisDirection);
    target->setPosition(originalPosition + axisDirection * moveAmount);
}

void TranslateHandle::setDragState(glm::vec3 initHitPos) {
    initialHitPoint = initHitPos;
    originalPosition = target->getPosition();
}
