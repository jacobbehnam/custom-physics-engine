#include "SceneObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/Scene.h>

SceneObject::SceneObject(Scene* scene, Mesh *meshPtr, Shader *sdr)
    : mesh(meshPtr), shader(sdr), ownerScene(scene) {
    ownerScene->addObject(this);
    boundingRadius = 1.0f;
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

bool SceneObject::rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 sphereCenter, float sphereRadius, float &outDistance) {
    // Assumes rayDir is normalized!
    // Math is solving for t when || P(t) - C || = r where P(t) = O + tD

    glm::vec3 OC = rayOrigin - sphereCenter;
    float b = 2.0f * glm::dot(OC, rayDir);
    float c = glm::dot(OC, OC) - sphereRadius * sphereRadius;
    float discriminant = b*b - 4*c;
    if (discriminant < 0.0f) return false;

    float sqrtDisc = glm::sqrt(discriminant);
    float t1 = (-b - sqrtDisc)/2.0f; // Entry ray
    float t2 = (-b + sqrtDisc)/2.0f; // Exit ray

    float t = (t1 >= 0.0f) ? t1 : t2; // If the ray began inside the object, only the exit value will be positive
    if (t < 0.0f) return false; // If both values are negative, then the object is behind the camera, so no intersection

    outDistance = t;
    return true;
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

float SceneObject::getBoundingRadius() const{
    return boundingRadius;
}

Shader* SceneObject::getShader() const {
    return shader;
}





