#include "TranslateHandle.h"

#include <iostream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SceneObject.h"
#include <graphics/MathUtils.h>

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
    : mesh(m), shader(sdr), target(tgt), axis(ax) {}

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

bool TranslateHandle::rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance) const{
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

void TranslateHandle::handleClick(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float distance) {
    std::cout << "click" << std::endl;
}


void TranslateHandle::draw() const {
    shader->use();

    glm::mat4 model = getModelMatrix();
    shader->setMat4("model", model);

    mesh->draw();
}

Shader *TranslateHandle::getShader() const {
    return shader;
}

