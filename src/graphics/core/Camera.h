#pragma once
#include <glm/glm.hpp>

struct CameraSettings {
    float movementSpeed = 3.0f;
    float mouseSensitivity = 0.05f;
    float fov = 45.0f;
};

enum class Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
};

class SceneObject;

class Camera {
public: // TODO: make these not public
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    double yaw = -90.0f;
    double pitch = 0.0f;

    Camera(glm::vec3 initPosition, CameraSettings settings = {});

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjMatrix() const;
    const CameraSettings& getSettings() const { return settings; }
    void setSettings(const CameraSettings& newSettings) { settings = newSettings; }

    void setAspectRatio(float ratio) { aspectRatio = ratio; }

    void setTarget(SceneObject* obj);
    void clearTarget();
    void update();

    void resetMouse();
    void processMouseMovement(float xoffset, float yoffset);

    void processKeyboard(Movement direction, float deltaTime);

    bool firstMouse  = true;
private:
    void updateCameraVectors();

    CameraSettings settings;
    float aspectRatio;
    SceneObject* targetObject = nullptr;
    glm::vec3 followOffset{20.0f, 15.0f, 30.0f};
};
