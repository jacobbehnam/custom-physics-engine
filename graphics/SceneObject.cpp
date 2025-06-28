#include "SceneObject.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/Scene.h>
#include <graphics/MathUtils.h>

SceneObject::SceneObject(Scene* scene, Mesh *meshPtr, Shader *sdr)
    : mesh(meshPtr), shader(sdr), ownerScene(scene) {
    ownerScene->addObject((IDrawable*)this);
    ownerScene->addObject((IPickable*)this);
}

glm::mat4 SceneObject::getModelMatrix() const {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotation.x, glm::vec3(1,0,0));
    model = glm::rotate(model, rotation.y, glm::vec3(0,1,0));
    model = glm::rotate(model, rotation.z, glm::vec3(0,0,1));
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
    if (ownerScene->translationGizmo) {
        delete ownerScene->translationGizmo;
        ownerScene->translationGizmo = nullptr;
    }

    ownerScene->translationGizmo = new Gizmo(ownerScene, mesh, this, shader);
}



void SceneObject::setPosition(const glm::vec3 &pos) {
    position = pos;
}

void SceneObject::setRotation(const glm::vec3 &rot) {
    rotation = rot;
}

void SceneObject::setScale(const glm::vec3 &scl) {
    scale = scl;
}

glm::vec3 SceneObject::getPosition() const{
    return position;
}

Shader* SceneObject::getShader() const {
    return shader;
}





