#include "TranslateHandle.h"

#include <iostream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "graphics/core/SceneObject.h"
#include <graphics/utils/MathUtils.h>

#include "graphics/core/ResourceManager.h"

TranslateHandle::TranslateHandle(Mesh *m, Shader* sdr, SceneObject *tgt, Axis ax, uint32_t objID)
    : mesh(m), shader(sdr), target(tgt), axis(ax), objectID(objID) {
    if (ResourceManager::loadMeshFromOBJ("../Arrow.obj", "arrow")) {
        mesh = ResourceManager::getMesh("arrow");
    } else {
        std::cout << "Unable to load obj" << std::endl;
    }
}

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
    return model;
}

void TranslateHandle::onDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir) {
    glm::vec3 axisDirection = axisDir(axis);
    // To solve for t, minimize the quantity || (rayOrig + rayDir * t) - initialHitPoint ||
    float t = glm::dot(-(rayOrig - initialHitPoint), rayDir);

    glm::vec3 delta = (rayOrig + rayDir * t) - initialHitPoint;
    // apply translation only along the axis direction component:
    float moveAmount = glm::dot(delta, axisDirection);
    if (target->rigidBody)
        target->rigidBody->setPosition(originalPosition + axisDirection * moveAmount);
    else
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

uint32_t TranslateHandle::getObjectID() const {
    return objectID;
}


void TranslateHandle::setDragState(glm::vec3 initHitPos) {
    initialHitPoint = initHitPos;
    originalPosition = target->getPosition();
}
