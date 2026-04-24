#include "GraphicsSettings.h"

namespace {
constexpr auto kGraphicsGroup = "graphics";
constexpr auto kUseRayTracedKey = "useRayTraced";
constexpr auto kRayTraceRequireGpuKey = "rayTraceRequireGpu";
constexpr auto kRayTraceResScaleKey = "rayTraceResolutionScale";
constexpr auto kEnableSunKey = "enableSun";
}

void GraphicsSettings::load(QSettings& settings) {
    settings.beginGroup(kGraphicsGroup);
    useRayTraced = settings.value(kUseRayTracedKey, useRayTraced).toBool();
    rayTraceRequireGpu = settings.value(kRayTraceRequireGpuKey, rayTraceRequireGpu).toBool();
    rayTraceResolutionScale = static_cast<float>(settings.value(kRayTraceResScaleKey, rayTraceResolutionScale).toDouble());
    enableSun = settings.value(kEnableSunKey, enableSun).toBool();
    if (rayTraceResolutionScale < 0.25f) {
        rayTraceResolutionScale = 0.25f;
    }
    if (rayTraceResolutionScale > 1.0f) {
        rayTraceResolutionScale = 1.0f;
    }
    settings.endGroup();
}

void GraphicsSettings::save(QSettings& settings) const {
    settings.beginGroup(kGraphicsGroup);
    settings.setValue(kUseRayTracedKey, useRayTraced);
    settings.setValue(kRayTraceRequireGpuKey, rayTraceRequireGpu);
    settings.setValue(kRayTraceResScaleKey, static_cast<double>(rayTraceResolutionScale));
    settings.setValue(kEnableSunKey, enableSun);
    settings.endGroup();
}