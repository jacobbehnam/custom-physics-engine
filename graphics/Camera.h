#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    enum Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
    };

    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    double yaw = -90.0f;
    double pitch = 0.0f;
    double sensitivity = 0.05f;

    float movementSpeed = 3.0f;
    float mouseSensitivity = 1.0f;
    float fov = 45.0f;

    Camera(glm::vec3 initPosition);

    glm::mat4 getViewMatrix() const;

    glm::mat4 getProjMatrix() const;

    void processMouseMovement(float xoffset, float yoffset);

    void processKeyboard(Movement direction, float deltaTime);
};
