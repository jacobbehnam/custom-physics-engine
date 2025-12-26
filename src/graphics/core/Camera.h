#pragma once
#include <glm/glm.hpp>

enum class Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
};

class Camera {
public: // TODO: make these not public
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    double yaw = -90.0f;
    double pitch = 0.0f;

    float movementSpeed = 3.0f;
    float mouseSensitivity = 0.05f;
    float fov = 45.0f;

    Camera(glm::vec3 initPosition);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjMatrix() const;

    void setAspectRatio(float ratio) { aspectRatio = ratio; }

    void resetMouse();
    void processMouseMovement(float xoffset, float yoffset);

    void processKeyboard(Movement direction, float deltaTime);

    bool firstMouse  = true;
private:
    float aspectRatio;
};
