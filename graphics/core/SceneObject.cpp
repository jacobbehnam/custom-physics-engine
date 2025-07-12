#include "SceneObject.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/core/Scene.h>
#include <graphics/utils/MathUtils.h>

#include "physics/PointMass.h"

SceneObject::SceneObject(Scene* scene, Mesh *meshPtr, Shader *sdr, bool wantPhysics, const glm::vec3& initPos)
    : mesh(meshPtr), shader(sdr), ownerScene(scene), objectID(scene->allocateObjectID()), position(initPos) {
    shader->use();
    shader->setVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));
    shader->setBool("isHovered", false);

    if (wantPhysics) {
        physicsBody = new Physics::PointMass(1.0f, position);
    }
    ownerScene->addObject(this);
}

SceneObject::~SceneObject() {
    ownerScene->freeObjectID(objectID);
}

glm::mat4 SceneObject::getModelMatrix() const{
    if (physicsBody) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, physicsBody->getPosition());
        model = model * glm::mat4_cast(orientation);
        model = glm::scale(model, scale);
        return model;
    }
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

bool SceneObject::intersectsAABB(const glm::vec3 &orig, const glm::vec3 &dir, float &outT) const {
    Physics::Bounding::AABB localAABB = getMesh()->getLocalAABB();
    bool hitSomething = false;
    float closestT = std::numeric_limits<float>::infinity();
    float outDistance = -std::numeric_limits<float>::max();
    if (localAABB.getTransformed(getModelMatrix()).intersectsRay(orig, dir, outT)) {
        if (outDistance < closestT) {
            closestT = outDistance;
            hitSomething = true;
        }
    }
    if (hitSomething)
        outT = closestT;

    return hitSomething;
}

bool SceneObject::intersectsMesh(const glm::vec3 &orig, const glm::vec3 &dir, float &outT) const {
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

        float outDistance = 0.0f;
        if (MathUtils::intersectTriangle(orig, dir, v0, v1, v2, outDistance)) {
            if (outDistance < closestT) {
                closestT = outDistance;
                hitSomething = true;
            }
        }
    }

    if (hitSomething) {
        outT = closestT;
    }
    return hitSomething;
}



bool SceneObject::rayIntersection(glm::vec3 orig, glm::vec3 dir, float &outT) {
    float tAABB;
    if (!intersectsAABB(orig, dir, tAABB))
        return false;

    float tTri;
    if (intersectsMesh(orig, dir, tTri)) {
        outT = tTri;
        return true;
    }

    return false;
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
    if (physicsBody)
        return physicsBody->getPosition();
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


