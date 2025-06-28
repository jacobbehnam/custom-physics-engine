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
    glm::vec3 axisOrigin = originalPosition - glm::vec3(1.0f,1.0f,1.0f);
    glm::vec3 u = axisDir(axis);
    glm::vec3 v = rayDir;
    glm::vec3 w0 = rayOrig - axisOrigin;

    float a = glm::dot(u,u);
    float b = glm::dot(u,v);
    float c = glm::dot(v,v);
    float d = glm::dot(u,w0);
    float e = glm::dot(v,w0);

    float denom = a*c - b*b;
    if (std::abs(denom) < 1e-6f) {
        // lines are nearly parallel; fallback: project w0 onto axis
        float t = d / a;
        glm::vec3 closest = axisOrigin + u * t;
        glm::vec3 delta   = closest - initialHitPoint;
        target->setPosition(originalPosition + delta);
        return;
    }

    // parameter along axis line:
    float t = (b*e - c*d) / denom;

    glm::vec3 newHitPoint = axisOrigin + u * t;
    glm::vec3 delta       = newHitPoint - initialHitPoint;
    std::cout << delta[0] << "," << delta[1] << "," << delta[2] << std::endl;

    // apply translation only along the axis direction component:
    float moveAmount = glm::dot(delta, axisDir(axis));
    target->setPosition(originalPosition + axisDir(axis) * moveAmount);
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

