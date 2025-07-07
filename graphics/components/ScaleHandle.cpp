#include "ScaleHandle.h"
#include <graphics/core/ResourceManager.h>
#include <iostream>

ScaleHandle::ScaleHandle(Mesh *m, Shader *sdr, SceneObject *tgt, Axis ax, uint32_t objID)
    : mesh(m), shader(sdr), target(tgt), axis(ax), objectID(objID) {
    if (ResourceManager::loadMeshFromOBJ("../Scale.obj", "scale")) {
        mesh = ResourceManager::getMesh("scale");
    } else {
        std::cout << "Unable to load obj" << std::endl;
    }
}

// TODO: might not need anymore
void ScaleHandle::draw() const {
    shader->use();

    std::vector<InstanceData> data;
    InstanceData a = {getModelMatrix(), objectID};
    data.push_back(a);

    mesh->drawInstanced(data);
}

Shader * ScaleHandle::getShader() const {
    return shader;
}

Mesh *ScaleHandle::getMesh() const {
    return mesh;
}


void ScaleHandle::onDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir) {
    glm::vec3 axisDirection = axisDir(axis);
    glm::vec3 localAxisDirection = target->getRotationQuat() * axisDirection;
    // To solve for t, minimize the quantity || (rayOrig + rayDir * t) - initialHitPoint ||
    float t = glm::dot(-(rayOrig - initialHitPoint), rayDir);

    glm::vec3 delta = (rayOrig + rayDir * t) - initialHitPoint;

    float scaleAmount = glm::dot(delta, localAxisDirection);
    target->setScale(originalScale + axisDirection * scaleAmount);
}

void ScaleHandle::setDragState(glm::vec3 initHitPos) {
    initialHitPoint = initHitPos;
    originalScale = target->getScale();
}

glm::mat4 ScaleHandle::getModelMatrix() const {
    glm::mat4 model(1.0f);
    glm::vec3 pos = target->getPosition();
    glm::quat rotation = target->getRotationQuat();
    model = glm::translate(model, pos);
    model = model * glm::mat4_cast(rotation);

    glm::vec3 dir = axisDir(axis);
    glm::vec3 from = {0,1,0}, to = dir;
    float cosA = glm::dot(from, to);
    glm::vec3 crossA = glm::cross(from, to);
    if (glm::length(crossA) > 1e-3f) {
        float angle = acos(glm::clamp(cosA, -1.0f, 1.0f));
        model = glm::rotate(model, angle, glm::normalize(crossA));
    }

    model = glm::scale(model, glm::vec3(thickness, length, thickness));
    return model;
}

uint32_t ScaleHandle::getObjectID() const {
    return objectID;
}
