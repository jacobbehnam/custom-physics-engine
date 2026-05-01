#include "Camera.h"

#include "SceneObject.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>

Camera::Camera(glm::vec3 initPosition) {
    position = initPosition;
    worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::cross(right, front);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getRenderViewMatrix() const {
    return glm::lookAt(glm::vec3(0.0f), front, up);
}

glm::mat4 Camera::getProjMatrix() const {
    return glm::perspective(
        glm::radians(fov),
        aspectRatio,
        nearClip,
        farClip
    );

}

void Camera::setClipRange(float nearPlane, float farPlane) {
    nearClip = std::max(nearPlane, 0.01f);
    farClip = std::max(farPlane, nearClip + 1.0f);
}

void Camera::setTarget(SceneObject* obj) {
    targetObject = obj;
}

void Camera::focusOn(SceneObject* obj) {
    if (!obj) return;

    const float visualRadius = glm::compMax(glm::abs(obj->getScale())) * 0.5f;
    const float padding = 1.25f;
    const float verticalHalfFov = glm::radians(fov) * 0.5f;
    const float horizontalHalfFov = std::atan(std::tan(verticalHalfFov) * std::max(aspectRatio, 0.01f));
    const float framingHalfFov = std::max(std::min(verticalHalfFov, horizontalHalfFov), glm::radians(1.0f));
    const float distance = std::max((visualRadius * padding) / std::sin(framingHalfFov), 0.35f);
    nearClip = std::max(distance - visualRadius * 2.0f, 0.01f);
    farClip = std::max(distance + visualRadius * 4.0f, 300000.0f);
    followOffset = glm::normalize(glm::vec3(1.0f, 0.55f, 1.0f)) * distance;

    const glm::vec3 targetPos = obj->getPosition();
    position = targetPos + followOffset;
    front = glm::normalize(targetPos - position);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
    yaw = glm::degrees(std::atan2(front.z, front.x));
    pitch = glm::degrees(std::asin(std::clamp(front.y, -1.0f, 1.0f)));
    targetObject = nullptr;
}

void Camera::clearTarget() {
    targetObject = nullptr;
    nearClip = 0.1f;
    farClip = 300000.0f;
    yaw = -90.0f;
    pitch = 0.0f;
    updateCameraVectors();
}

void Camera::update() {
    if (targetObject) {
        glm::vec3 targetPos = targetObject->getPosition();

        position = targetPos + followOffset;

        front = glm::normalize(targetPos - position);
        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
    yaw += xoffset * this->mouseSensitivity;
    pitch += yoffset * this->mouseSensitivity;

    if (pitch > 90)
        pitch = 90;
    if (pitch < -90)
        pitch = -90;

    updateCameraVectors();
}

void Camera::processKeyboard(Movement direction, float deltaTime) {
    float velocity = this->movementSpeed * deltaTime;
    switch (direction) {
        case Movement::FORWARD:
            position += front * velocity;
            break;
        case Movement::BACKWARD:
            position -= front * velocity;
            break;
        case Movement::LEFT:
            position -= right * velocity;
            break;
        case Movement::RIGHT:
            position += right * velocity;
            break;
    }
}

void Camera::updateCameraVectors() {
    glm::vec3 newDir;
    newDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newDir.y = sin(glm::radians(pitch));
    newDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newDir);
    right = glm::normalize(glm::cross(front, worldUp));
    up    = glm::normalize(glm::cross(right, front));
}

void Camera::resetMouse() {
    firstMouse = true;
}
