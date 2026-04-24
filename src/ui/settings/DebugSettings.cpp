#include "DebugSettings.h"

namespace {
constexpr auto kDebugGroup = "debug";
constexpr auto kShowAllPathTrailsKey = "showAllPathTrails";
constexpr auto kPathTrailTimeKey = "pathTrailTime";
constexpr auto kShowForcesKey = "showForces";
constexpr auto kShowCollidersKey = "showColliders";
constexpr auto kUseRayTracedKey = "useRayTraced";
constexpr auto kRayTraceRequireGpuKey = "rayTraceRequireGpu";
constexpr auto kRayTraceResScaleKey = "rayTraceResolutionScale";
}

void DebugSettings::load(QSettings& settings) {
    settings.beginGroup(kDebugGroup);
    showAllPathTrails = settings.value(kShowAllPathTrailsKey, showAllPathTrails).toBool();
    pathTrailTime = settings.value(kPathTrailTimeKey, pathTrailTime).toFloat();
    showForces = settings.value(kShowForcesKey, showForces).toBool();
    showColliders = settings.value(kShowCollidersKey, showColliders).toBool();
    useRayTraced = settings.value(kUseRayTracedKey, useRayTraced).toBool();
    rayTraceRequireGpu = settings.value(kRayTraceRequireGpuKey, rayTraceRequireGpu).toBool();
    rayTraceResolutionScale = static_cast<float>(settings.value(kRayTraceResScaleKey, rayTraceResolutionScale).toDouble());
    if (rayTraceResolutionScale < 0.25f) {
        rayTraceResolutionScale = 0.25f;
    }
    if (rayTraceResolutionScale > 1.0f) {
        rayTraceResolutionScale = 1.0f;
    }
    settings.endGroup();
}

void DebugSettings::save(QSettings& settings) const {
    settings.beginGroup(kDebugGroup);
    settings.setValue(kShowAllPathTrailsKey, showAllPathTrails);
    settings.setValue(kPathTrailTimeKey, pathTrailTime);
    settings.setValue(kShowForcesKey, showForces);
    settings.setValue(kShowCollidersKey, showColliders);
    settings.setValue(kUseRayTracedKey, useRayTraced);
    settings.setValue(kRayTraceRequireGpuKey, rayTraceRequireGpu);
    settings.setValue(kRayTraceResScaleKey, static_cast<double>(rayTraceResolutionScale));
    settings.endGroup();
}
