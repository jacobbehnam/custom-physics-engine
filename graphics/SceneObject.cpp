#include "SceneObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

SceneObject::SceneObject(Mesh *meshPtr, unsigned int program)
    : mesh(meshPtr), shaderProgram(program) {
    sceneObjects.push_back(this);
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
    glUseProgram(shaderProgram);

    glm::mat4 model = getModelMatrix();
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    mesh->draw();
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




