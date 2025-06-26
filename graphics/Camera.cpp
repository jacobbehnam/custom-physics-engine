#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 initPosition) {
    position = initPosition;
    worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    front = glm::normalize(-position);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::cross(right, front);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjMatrix() const {
    return glm::perspective(
        glm::radians(fov),
        (float) 800 / 600,
        0.1f,
        100.0f
);
}
void Camera::processMouseMovement(float xoffset, float yoffset) {
    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    if (pitch > 90)
        pitch = 90;
    if (pitch < -90)
        pitch = -90;

    glm::vec3 newDir;
    newDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newDir.y = sin(glm::radians(pitch));
    newDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = newDir;
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::cross(right, front);
}

void Camera::processKeyboard(Movement direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
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