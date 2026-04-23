#include "CameraSettingsGroup.h"

namespace {
constexpr auto kCameraGroup = "camera";
constexpr auto kMovementSpeedKey = "movementSpeed";
constexpr auto kMouseSensitivityKey = "mouseSensitivity";
constexpr auto kFovKey = "fov";
}

void CameraSettingsGroup::load(QSettings& settings) {
    settings.beginGroup(kCameraGroup);
    movementSpeed = settings.value(kMovementSpeedKey, movementSpeed).toFloat();
    mouseSensitivity = settings.value(kMouseSensitivityKey, mouseSensitivity).toFloat();
    fov = settings.value(kFovKey, fov).toFloat();
    settings.endGroup();
}

void CameraSettingsGroup::save(QSettings& settings) const {
    settings.beginGroup(kCameraGroup);
    settings.setValue(kMovementSpeedKey, movementSpeed);
    settings.setValue(kMouseSensitivityKey, mouseSensitivity);
    settings.setValue(kFovKey, fov);
    settings.endGroup();
}
