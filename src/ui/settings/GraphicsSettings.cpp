#include "GraphicsSettings.h"

namespace {
constexpr auto kGraphicsGroup = "graphics";
constexpr auto kUseRayTracedKey = "useRayTraced";
constexpr auto kRayTraceResScaleKey = "rayTraceResolutionScale";
constexpr auto kEnableGlobalLightKey = "enableGlobalLight";
}

void GraphicsSettings::load(QSettings& settings) {
    settings.beginGroup(kGraphicsGroup);
    useRayTraced = settings.value(kUseRayTracedKey, useRayTraced).toBool();
    rayTraceResolutionScale = static_cast<float>(settings.value(kRayTraceResScaleKey, rayTraceResolutionScale).toDouble());
    enableGlobalLight = settings.value(kEnableGlobalLightKey, enableGlobalLight).toBool();
    if (rayTraceResolutionScale < kMinRayTraceResolutionScale) {
        rayTraceResolutionScale = kMinRayTraceResolutionScale;
    }
    if (rayTraceResolutionScale > kMaxRayTraceResolutionScale) {
        rayTraceResolutionScale = kMaxRayTraceResolutionScale;
    }
    settings.endGroup();
}

void GraphicsSettings::save(QSettings& settings) const {
    settings.beginGroup(kGraphicsGroup);
    settings.setValue(kUseRayTracedKey, useRayTraced);
    settings.setValue(kRayTraceResScaleKey, static_cast<double>(rayTraceResolutionScale));
    settings.setValue(kEnableGlobalLightKey, enableGlobalLight);
    settings.endGroup();
}
