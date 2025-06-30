#include "TranslateHandle.h"

#include <iostream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "graphics/core/SceneObject.h"
#include <graphics/utils/MathUtils.h>

static glm::vec3 axisDir(Axis a) {
    switch (a) {
        case Axis::X:
            return {1,0,0};
            break;
        case Axis::Y:
            return {0,1,0};
            break;
        default:
            return {0,0,1};
    }
}

TranslateHandle::TranslateHandle(Mesh *m, Shader* sdr, SceneObject *tgt, Axis ax)
    : mesh(m), shader(sdr), target(tgt), axis(ax){}

glm::mat4 TranslateHandle::getModelMatrix() const {
    glm::mat4 model(1.0f);
    glm::vec3 pos = target->getPosition();
    model = glm::translate(model, pos);

    glm::vec3 dir = axisDir(axis);
    glm::vec3 from = {0,1,0}, to = dir; // We assume the TranslateHandle mesh points in the +Y axis direction.
    float cosA = glm::dot(from, to);
    glm::vec3 crossA = glm::cross(from, to);
    if (glm::length(crossA) > 1e-3f) {
        float angle = acos(glm::clamp(cosA, -1.0f, 1.0f));
        model = glm::rotate(model, angle, glm::normalize(crossA));
    }

    model = glm::scale(model, glm::vec3(thickness, length, thickness));
    model = glm::translate(model, glm::vec3(1.0f, 1.0f, 1.0f));
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


void TranslateHandle::draw() const {
    shader->use();

    glm::mat4 model = getModelMatrix();
    shader->setMat4("model", model);

    mesh->draw();
}

Shader* TranslateHandle::getShader() const {
    return shader;
}

Mesh* TranslateHandle::getMesh() const {
    return mesh;
}

void TranslateHandle::setDragState(glm::vec3 initHitPos, glm::vec3 originPos) {
    initialHitPoint = initHitPos;
    originalPosition = originPos;
}
