#pragma once

#include <QSettings>
#include "graphics/core/Camera.h"

struct AppSettings {
    CameraSettings camera;

    static AppSettings load(QSettings& settings);
    void save(QSettings& settings) const;
};
