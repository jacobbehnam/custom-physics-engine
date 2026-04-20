#pragma once
#include "ISettingsGroup.h"
#include "graphics/core/Camera.h"

struct CameraSettingsGroup : public ISettingsGroup {
    float movementSpeed = Camera::kDefaultMovementSpeed;
    float mouseSensitivity = Camera::kDefaultMouseSensitivity;
    float fov = Camera::kDefaultFov;

    CameraSettingsGroup() = default;

    void load(QSettings& settings) override;
    void save(QSettings& settings) const override;
};
