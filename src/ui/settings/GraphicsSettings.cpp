#include "GraphicsSettings.h"

#include <cmath>

namespace {
constexpr auto kGraphicsGroup = "graphics";
constexpr auto kUseRayTracedKey = "useRayTraced";
constexpr auto kRayTraceResScaleKey = "rayTraceResolutionScale";
constexpr auto kRayTraceExposureKey = "rayTraceExposure";
constexpr auto kEnableGlobalLightKey = "enableGlobalLight";
}

void GraphicsSettings::load(QSettings& settings) {
    settings.beginGroup(kGraphicsGroup);
    useRayTraced = settings.value(kUseRayTracedKey, useRayTraced).toBool();
    rayTraceResolutionScale = static_cast<float>(settings.value(kRayTraceResScaleKey, rayTraceResolutionScale).toDouble());
    rayTraceExposure = static_cast<float>(settings.value(kRayTraceExposureKey, rayTraceExposure).toDouble());
    enableGlobalLight = settings.value(kEnableGlobalLightKey, enableGlobalLight).toBool();
    if (!std::isfinite(rayTraceResolutionScale)) {
        rayTraceResolutionScale = kMaxRayTraceResolutionScale;
    }
    if (rayTraceResolutionScale < kMinRayTraceResolutionScale) {
        rayTraceResolutionScale = kMinRayTraceResolutionScale;
    }
    if (rayTraceResolutionScale > kMaxRayTraceResolutionScale) {
        rayTraceResolutionScale = kMaxRayTraceResolutionScale;
    }
    if (!std::isfinite(rayTraceExposure)) {
        rayTraceExposure = 1.0f;
    }
    if (rayTraceExposure < kMinRayTraceExposure) {
        rayTraceExposure = kMinRayTraceExposure;
    }
    if (rayTraceExposure > kMaxRayTraceExposure) {
        rayTraceExposure = kMaxRayTraceExposure;
    }
    settings.endGroup();
}

void GraphicsSettings::save(QSettings& settings) const {
    settings.beginGroup(kGraphicsGroup);
    settings.setValue(kUseRayTracedKey, useRayTraced);
    settings.setValue(kRayTraceResScaleKey, static_cast<double>(rayTraceResolutionScale));
    settings.setValue(kRayTraceExposureKey, static_cast<double>(rayTraceExposure));
    settings.setValue(kEnableGlobalLightKey, enableGlobalLight);
    settings.endGroup();
}
