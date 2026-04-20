#pragma once
#include "ISettingsGroup.h"

struct CameraSettingsGroup : public ISettingsGroup {
    float movementSpeed = 3.0f;
    float mouseSensitivity = 0.05f;
    float fov = 45.0f;

    CameraSettingsGroup() = default;

    void load(QSettings& settings) override;
    void save(QSettings& settings) const override;
};
