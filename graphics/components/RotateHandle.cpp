#include "RotateHandle.h"

#include <iostream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "graphics/core/SceneObject.h"
#include <graphics/utils/MathUtils.h>
#include <graphics/core/ResourceManager.h>


RotateHandle::RotateHandle(Mesh *m, Shader *sdr, SceneObject *tgt, Axis ax)
    : mesh(m), shader(sdr), target(tgt), axis(ax) {
    if (ResourceManager::loadMeshFromOBJ("../Rotate.obj", "rotate")) {
        mesh = ResourceManager::getMesh("rotate");
    } else {
        std::cout << "Unable to load obj" << std::endl;
    }
}

void RotateHandle::draw() const {
    shader->use();

    glm::mat4 model = getModelMatrix();
    shader->setMat4("model", model);

    mesh->draw();
}

Shader * RotateHandle::getShader() const {
    return shader;
}

void RotateHandle::onDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir) {
    glm::vec3 axisDirection = axisDir(axis);
    glm::vec3& planeNormal = axisDirection;
    glm::vec3 center = target->getPosition();

    // Ray-plane intersection
    float denom = glm::dot(rayDir, planeNormal);
    if (glm::abs(denom) < 1e-6f) return; // ray is parallel to plane

    float t = glm::dot(center - rayOrig, planeNormal) / denom;
    glm::vec3 currentHitPoint = rayOrig + t * rayDir;

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
    glm::vec3 pos = target->getPosition();
    model = glm::translate(model, pos);

    glm::vec3 dir = axisDir(axis);
    glm::vec3 from = {0,1,0}, to = dir;
    float cosA = glm::dot(from, to);
    glm::vec3 crossA = glm::cross(from, to);
    if (glm::length(crossA) > 1e-3f) {
        float angle = acos(glm::clamp(cosA, -1.0f, 1.0f));
        model = glm::rotate(model, angle, glm::normalize(crossA));
    }

    model = glm::scale(model, glm::vec3(scale, scale, scale));
    return model;
}

Mesh * RotateHandle::getMesh() const {
    return mesh;
}
