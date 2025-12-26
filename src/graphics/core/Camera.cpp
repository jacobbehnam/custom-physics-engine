#include "Camera.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

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

glm::mat4 Camera::getProjMatrix() const {
    return glm::perspective(
        glm::radians(fov),
        aspectRatio,
        0.1f,
        100.0f
);
}

// void Camera::handleMouseMovement(double xpos, double ypos) {
//     if (firstMouse) {
//         lastX = (float)xpos;
//         lastY = (float)ypos;
//         firstMouse = false;
//     }
//
//     // Calculate movement offset
//     float xoffset = (float)xpos - lastX;
//     float yoffset = lastY - (float)ypos; // reversed since y-coordinates go from bottom to top
//
//     lastX = (float)xpos;
//     lastY = (float)ypos;
//
//     processMouseMovement(xoffset, yoffset);
// }


void Camera::processMouseMovement(float xoffset, float yoffset) {
    yaw += xoffset * mouseSensitivity;
    pitch += yoffset * mouseSensitivity;

    if (pitch > 90)
        pitch = 90;
    if (pitch < -90)
        pitch = -90;

    glm::vec3 newDir;
    newDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newDir.y = sin(glm::radians(pitch));
    newDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newDir);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
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

void Camera::resetMouse() {
    firstMouse = true;
}
