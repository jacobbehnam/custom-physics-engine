#include "DebugSettings.h"

namespace {
constexpr auto kDebugGroup = "debug";
constexpr auto kShowAllPathTrailsKey = "showAllPathTrails";
constexpr auto kPathTrailTimeKey = "pathTrailTime";
constexpr auto kShowForcesKey = "showForces";
constexpr auto kShowCollidersKey = "showColliders";
constexpr auto kShowObjectLabelsKey = "showObjectLabels";
}

void DebugSettings::load(QSettings& settings) {
    settings.beginGroup(kDebugGroup);
    showAllPathTrails = settings.value(kShowAllPathTrailsKey, showAllPathTrails).toBool();
    pathTrailTime = settings.value(kPathTrailTimeKey, pathTrailTime).toFloat();
    showForces = settings.value(kShowForcesKey, showForces).toBool();
    showColliders = settings.value(kShowCollidersKey, showColliders).toBool();
    showObjectLabels = settings.value(kShowObjectLabelsKey, showObjectLabels).toBool();
    settings.endGroup();
}

void DebugSettings::save(QSettings& settings) const {
    settings.beginGroup(kDebugGroup);
    settings.setValue(kShowAllPathTrailsKey, showAllPathTrails);
    settings.setValue(kPathTrailTimeKey, pathTrailTime);
    settings.setValue(kShowForcesKey, showForces);
    settings.setValue(kShowCollidersKey, showColliders);
    settings.setValue(kShowObjectLabelsKey, showObjectLabels);
    settings.endGroup();
}
