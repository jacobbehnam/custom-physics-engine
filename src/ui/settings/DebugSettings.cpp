#include "DebugSettings.h"

namespace {
constexpr auto kDebugGroup = "debug";
constexpr auto kShowAllPathTrailsKey = "showAllPathTrails";
constexpr auto kPathTrailTimeKey = "pathTrailTime";
}

void DebugSettings::load(QSettings& settings) {
    settings.beginGroup(kDebugGroup);
    showAllPathTrails = settings.value(kShowAllPathTrailsKey, showAllPathTrails).toBool();
    pathTrailTime = settings.value(kPathTrailTimeKey, pathTrailTime).toFloat();
    settings.endGroup();
}

void DebugSettings::save(QSettings& settings) const {
    settings.beginGroup(kDebugGroup);
    settings.setValue(kShowAllPathTrailsKey, showAllPathTrails);
    settings.setValue(kPathTrailTimeKey, pathTrailTime);
    settings.endGroup();
}
