#pragma once
#include <glm/glm.hpp>

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

    static constexpr float kDefaultMovementSpeed = 3.0f;
    static constexpr float kDefaultMouseSensitivity = 0.05f;
    static constexpr float kDefaultFov = 45.0f;
    static constexpr float kDefaultNearClip = 0.1f;
    static constexpr float kDefaultFarClip = 300000.0f;

    float movementSpeed = kDefaultMovementSpeed;
    float mouseSensitivity = kDefaultMouseSensitivity;
    float fov = kDefaultFov;

    Camera(glm::vec3 initPosition);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getRenderViewMatrix() const;
    glm::mat4 getProjMatrix() const;

    void setAspectRatio(float ratio) { aspectRatio = ratio; }
    void setClipRange(float nearPlane, float farPlane);
    void setView(const glm::vec3& newPosition, double newYaw, double newPitch);
    void resetView(const glm::vec3& newPosition = glm::vec3(0.0f, 10.0f, 30.0f));

    void setTarget(SceneObject* obj);
    void focusOn(SceneObject* obj);
    void clearTarget();
    bool hasTarget() const { return targetObject != nullptr; }
    const SceneObject* getTarget() const { return targetObject; }
    void update(const glm::vec3* renderTargetPosition = nullptr);

    void resetMouse();
    void processMouseMovement(float xoffset, float yoffset);
    void processScroll(float wheelSteps);

    void processKeyboard(Movement direction, float deltaTime);

    bool firstMouse  = true;
private:
    void updateCameraVectors();

    float aspectRatio = 16.0f / 9.0f;
    float nearClip = kDefaultNearClip;
    float farClip = kDefaultFarClip;
    SceneObject* targetObject = nullptr;
    glm::vec3 followOffset{20.0f, 15.0f, 30.0f};
    glm::vec3 followPivot{0.0f};
};
