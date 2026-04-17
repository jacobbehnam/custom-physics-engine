#include "AppSettings.h"

#include "graphics/core/Camera.h"

namespace {
constexpr auto kCameraGroup = "camera";
constexpr auto kMovementSpeedKey = "movementSpeed";
constexpr auto kMouseSensitivityKey = "mouseSensitivity";
constexpr auto kFovKey = "fov";
}

AppSettings AppSettings::load(QSettings& settings) {
    AppSettings appSettings;

    settings.beginGroup(kCameraGroup);
    appSettings.camera.movementSpeed = settings.value(kMovementSpeedKey, appSettings.camera.movementSpeed).toFloat();
    appSettings.camera.mouseSensitivity = settings.value(kMouseSensitivityKey, appSettings.camera.mouseSensitivity).toFloat();
    appSettings.camera.fov = settings.value(kFovKey, appSettings.camera.fov).toFloat();
    settings.endGroup();

    return appSettings;
}

void AppSettings::save(QSettings& settings) const {
    settings.beginGroup(kCameraGroup);
    settings.setValue(kMovementSpeedKey, camera.movementSpeed);
    settings.setValue(kMouseSensitivityKey, camera.mouseSensitivity);
    settings.setValue(kFovKey, camera.fov);
    settings.endGroup();
}
