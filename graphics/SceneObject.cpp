#include "SceneObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/Scene.h>

// Helper function that implements the Moller–Trumbore ray/triangle intersection algorithm
bool intersectTriangle(
    const glm::vec3& orig,
    const glm::vec3& dir,
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2,
    float& outT
) {
    const float EPSILON = 1e-8f;

    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;

    glm::vec3 pvec = glm::cross(dir, edge2);
    float det = glm::dot(edge1, pvec);

    // If the determinant is near zero, ray lies in plane of triangle or is backfacing
    if (std::abs(det) < EPSILON) return false;

    float invDet = 1.0f / det;

    glm::vec3 tvec = orig - v0;
    float u = glm::dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 qvec = glm::cross(tvec, edge1);
    float v = glm::dot(dir, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;

    // compute t to find out where the intersection point is on the line
    float t = glm::dot(edge2, qvec) * invDet;
    if (t < EPSILON) return false;  // intersection is behind the ray origin

    outT = t;
    return true;
}

SceneObject::SceneObject(Scene* scene, Mesh *meshPtr, Shader *sdr)
    : mesh(meshPtr), shader(sdr), ownerScene(scene) {
    ownerScene->addObject(this);
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

    for (int i = 0; i + 2 < indices.size(); i += 3) {
        const glm::vec3& v0 = verts[indices[i]].pos;
        const glm::vec3& v1 = verts[indices[i+1]].pos;
        const glm::vec3& v2 = verts[indices[i+2]].pos;

        float outT;
        if (intersectTriangle(rayOrigin, rayDir, v0, v1, v2, outT)) {
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





