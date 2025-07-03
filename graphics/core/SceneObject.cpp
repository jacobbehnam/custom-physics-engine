#include "SceneObject.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/core/Scene.h>
#include <graphics/utils/MathUtils.h>

SceneObject::SceneObject(Scene* scene, Mesh *meshPtr, Shader *sdr)
    : mesh(meshPtr), shader(sdr), ownerScene(scene), objectID(scene->allocateObjectID()) {
    shader->use();
    shader->setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
    shader->setBool("isHovered", false);
    ownerScene->addObject((IDrawable*)this);
    ownerScene->addObject((IPickable*)this);
}

glm::mat4 SceneObject::getModelMatrix() const{
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = model * glm::mat4_cast(orientation);
    model = glm::scale(model, scale);
    return model;
}

void SceneObject::draw() const {
    shader->use();

    glm::mat4 model = getModelMatrix();
    shader->setMat4("model", model);

    mesh->draw();
}

bool SceneObject::rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance) {
    bool hitSomething = false;
    float closestT = std::numeric_limits<float>::infinity();

    const std::vector<Vertex>& verts = mesh->getVertices();
    const std::vector<unsigned int>& indices = mesh->getIndices();
    const glm::mat4 model = getModelMatrix();

    for (int i = 0; i + 2 < indices.size(); i += 3) {
        // Converting local coordinates of the mesh to world coordinates can probably be optimized with the GPU
        const glm::vec3& v0 = glm::vec3(model * glm::vec4(verts[indices[i]].pos, 1));
        const glm::vec3& v1 = glm::vec3(model * glm::vec4(verts[indices[i+1]].pos, 1));
        const glm::vec3& v2 = glm::vec3(model * glm::vec4(verts[indices[i+2]].pos, 1));

        float outT;
        if (MathUtils::intersectTriangle(rayOrigin, rayDir, v0, v1, v2, outT)) {
            if (outT < closestT) {
                closestT = outT;
                hitSomething = true;
            }
        }
    }

    if (hitSomething) {
        outDistance = closestT;
    }
    return hitSomething;
}

void SceneObject::handleClick(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float distance) {
    ownerScene->setGizmoFor(this);
}

void SceneObject::setPosition(const glm::vec3 &pos) {
    position = pos;
}

void SceneObject::setRotation(const glm::vec3 &euler) {
    orientation = glm::quat(euler);
}

void SceneObject::setScale(const glm::vec3 &scl) {
    scale = scl;
}

glm::vec3 SceneObject::getPosition() const{
    return position;
}

glm::vec3 SceneObject::getRotation() const {
    return glm::eulerAngles(orientation);
}

Shader* SceneObject::getShader() const {
    return shader;
}

void SceneObject::setRotationQuat(const glm::quat &q) {
    orientation = q;
}
glm::quat SceneObject::getRotationQuat()   const {
    return orientation;
}

glm::vec3 SceneObject::getScale() const {
    return scale;
}

void SceneObject::setHovered(bool hovered) {
    isHovered = hovered;
}

bool SceneObject::getHovered() {
    return isHovered;
}

uint32_t SceneObject::getObjectID() const {
    return objectID;
}


