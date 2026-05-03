#include "Camera.h"

#include "SceneObject.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>

namespace {
constexpr float kMinAspectRatio                = 0.0001f;
constexpr float kMinFollowOffsetLengthSq       = 0.000001f;
constexpr float kMinFollowOrbitRadius          = 0.000001f;
constexpr float kMinAspectRatioForFocus        = 0.01f;
constexpr float kMinFocusFramingAngleDeg       = 1.0f;
constexpr float kFocusPadding                  = 1.25f;
constexpr float kMinFocusDistance              = 0.35f;
constexpr glm::vec3 kFocusDirection            = glm::vec3(1.0f, 0.55f, 1.0f);
constexpr float kMinFarClipSeparation          = 1.0f;
constexpr double kPitchClampDeg                = 90.0;
constexpr float kTargetPitchClampDeg           = 89.0f;
constexpr float kMinScrollTargetRadius         = 0.01f;
constexpr float kMinScrollDistanceMultiplier   = 1.05f;
constexpr float kMaxScrollDistanceMultiplier   = 1000000.0f;
constexpr float kTargetZoomFactorPerWheelStep  = 0.85f;
constexpr double kDefaultYawDeg                = -90.0;
constexpr double kDefaultPitchDeg              = 0.0;
constexpr float kDefaultFollowOffsetDistance   = 1.0f;
}

Camera::Camera(glm::vec3 initPosition) {
    position = initPosition;
    worldUp  = glm::vec3(0.0f, 1.0f, 0.0f);
    front    = glm::vec3(0.0f, 0.0f, -1.0f);
    right    = glm::normalize(glm::cross(front, worldUp));
    up       = glm::cross(right, front);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getRenderViewMatrix() const {
    return glm::lookAt(glm::vec3(0.0f), front, up);
}

glm::mat4 Camera::getProjMatrix() const {
    const float tanHalfFov = std::tan(glm::radians(fov) * 0.5f);
    const float safeAspect = std::max(aspectRatio, kMinAspectRatio);

    glm::mat4 projection(0.0f);
    projection[0][0] = 1.0f / (safeAspect * tanHalfFov);
    projection[1][1] = 1.0f / tanHalfFov;
    projection[2][3] = -1.0f;
    projection[3][2] = nearClip;
    return projection;
}

void Camera::setClipRange(float nearPlane, float farPlane) {
    nearClip = std::max(nearPlane, kDefaultNearClip);
    farClip  = std::max(farPlane, nearClip + kMinFarClipSeparation);
}

void Camera::setView(const glm::vec3& newPosition, double newYaw, double newPitch) {
    targetObject = nullptr;
    position     = newPosition;
    yaw          = newYaw;
    pitch        = std::clamp(newPitch, -kPitchClampDeg, kPitchClampDeg);
    nearClip     = kDefaultNearClip;
    farClip      = kDefaultFarClip;
    resetMouse();
    updateCameraVectors();
}

void Camera::resetView(const glm::vec3& newPosition) {
    setView(newPosition, kDefaultYawDeg, kDefaultPitchDeg);
}

void Camera::setTarget(SceneObject* obj) {
    targetObject = obj;
    if (targetObject) {
        followPivot  = targetObject->getPosition();
        followOffset = position - followPivot;
        if (glm::length2(followOffset) <= kMinFollowOffsetLengthSq) {
            followOffset = -front * kDefaultFollowOffsetDistance;
        }
    }
}

void Camera::focusOn(SceneObject* obj) {
    if (!obj) return;

    const float visualRadius      = glm::compMax(glm::abs(obj->getScale())) * 0.5f;
    const float verticalHalfFov   = glm::radians(fov) * 0.5f;
    const float horizontalHalfFov = std::atan(std::tan(verticalHalfFov) * std::max(aspectRatio, kMinAspectRatioForFocus));
    const float framingHalfFov    = std::max(std::min(verticalHalfFov, horizontalHalfFov), glm::radians(kMinFocusFramingAngleDeg));
    const float distance          = std::max((visualRadius * kFocusPadding) / std::sin(framingHalfFov), kMinFocusDistance);
    nearClip                      = std::max(distance - visualRadius * 2.0f, kDefaultNearClip);
    farClip                       = std::max(distance + visualRadius * 4.0f, kDefaultFarClip);
    followOffset                  = glm::normalize(kFocusDirection) * distance;

    const glm::vec3 targetPos = obj->getPosition();
    followPivot  = targetPos;
    position     = targetPos + followOffset;
    front        = glm::normalize(targetPos - position);
    right        = glm::normalize(glm::cross(front, worldUp));
    up           = glm::normalize(glm::cross(right, front));
    yaw          = glm::degrees(std::atan2(front.z, front.x));
    pitch        = glm::degrees(std::asin(std::clamp(front.y, -1.0f, 1.0f)));
    targetObject = nullptr;
}

void Camera::clearTarget() {
    targetObject = nullptr;
    nearClip = kDefaultNearClip;
    farClip = kDefaultFarClip;
}

void Camera::update(const glm::vec3* renderTargetPosition) {
    if (targetObject) {
        glm::vec3 targetPos = renderTargetPosition ? *renderTargetPosition : targetObject->getPosition();
        if (!std::isfinite(targetPos.x) || !std::isfinite(targetPos.y) || !std::isfinite(targetPos.z)) {
            return;
        }

        followPivot = targetPos;
        position    = targetPos + followOffset;

        front = glm::normalize(targetPos - position);
        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
    if (targetObject) {
        glm::vec3 toCamera   = followOffset;
        const float radius   = std::max(glm::length(toCamera), kMinFollowOrbitRadius);
        glm::vec3 direction  = glm::normalize(toCamera);
        float orbitYaw       = std::atan2(direction.z, direction.x);
        float orbitPitch   = std::asin(std::clamp(direction.y, -1.0f, 1.0f));
        orbitYaw          += glm::radians(xoffset * mouseSensitivity);
        orbitPitch -= glm::radians(yoffset * mouseSensitivity);
        orbitPitch        = std::clamp(orbitPitch, glm::radians(-kTargetPitchClampDeg), glm::radians(kTargetPitchClampDeg));

        direction.x  = std::cos(orbitYaw) * std::cos(orbitPitch);
        direction.y  = std::sin(orbitPitch);
        direction.z  = std::sin(orbitYaw) * std::cos(orbitPitch);
        followOffset = glm::normalize(direction) * radius;
        position     = followPivot + followOffset;
        front        = glm::normalize(followPivot - position);
        right        = glm::normalize(glm::cross(front, worldUp));
        up           = glm::normalize(glm::cross(right, front));
        yaw          = glm::degrees(std::atan2(front.z, front.x));
        pitch        = glm::degrees(std::asin(std::clamp(front.y, -1.0f, 1.0f)));
        return;
    }

    yaw   += xoffset * this->mouseSensitivity;
    pitch += yoffset * this->mouseSensitivity;

    if (pitch > kPitchClampDeg)
        pitch = kPitchClampDeg;
    if (pitch < -kPitchClampDeg)
        pitch = -kPitchClampDeg;

    updateCameraVectors();
}

void Camera::processScroll(float wheelSteps) {
    if (wheelSteps == 0.0f) return;

    if (targetObject) {
        const float targetRadius    = std::max(glm::compMax(glm::abs(targetObject->getScale())) * 0.5f, kMinScrollTargetRadius);
        const float minDistance     = targetRadius * kMinScrollDistanceMultiplier;
        const float maxDistance     = std::max(targetRadius * kMaxScrollDistanceMultiplier, minDistance + kMinFarClipSeparation);
        const float currentDistance = std::max(glm::length(followOffset), minDistance);
        const float zoomFactor      = std::pow(kTargetZoomFactorPerWheelStep, wheelSteps);
        const float newDistance     = std::clamp(currentDistance * zoomFactor, minDistance, maxDistance);

        glm::vec3 direction = glm::length2(followOffset) > kMinFollowOffsetLengthSq ? glm::normalize(followOffset) : -front;
        followOffset = direction * newDistance;
        position     = followPivot + followOffset;
        front        = glm::normalize(followPivot - position);
        right        = glm::normalize(glm::cross(front, worldUp));
        up           = glm::normalize(glm::cross(right, front));
        return;
    }

    position += front * movementSpeed * wheelSteps;
}

void Camera::processKeyboard(Movement direction, float deltaTime) {
    float velocity = this->movementSpeed * deltaTime;
    if (targetObject) {
        (void)direction;
        (void)velocity;
        return;
    }

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
    front    = glm::normalize(newDir);
    right    = glm::normalize(glm::cross(front, worldUp));
    up       = glm::normalize(glm::cross(right, front));
}

void Camera::resetMouse() {
    firstMouse = true;
}
