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
    //TODO
}

void RotateHandle::setDragState(glm::vec3 initHitPos, glm::vec3 originPos) {
    //TODO
}

glm::mat4 RotateHandle::getModelMatrix() const {
    glm::mat4 model(1.0f);
    glm::vec3 pos = target->getPosition();
    model = glm::translate(model, pos);

    glm::vec3 dir = axisDir(axis);
    glm::vec3 from = {0,1,0}, to = dir; // We assume the handle mesh points in the +Y axis direction.
    float cosA = glm::dot(from, to);
    glm::vec3 crossA = glm::cross(from, to);
    if (glm::length(crossA) > 1e-3f) {
        float angle = acos(glm::clamp(cosA, -1.0f, 1.0f));
        model = glm::rotate(model, angle, glm::normalize(crossA));
    }

    model = glm::scale(model, glm::vec3(scale, scale, scale));
    // model = glm::translate(model, glm::vec3(1.0f, 1.0f, 1.0f));
    return model;
}

Mesh * RotateHandle::getMesh() const {
    return mesh;
}
