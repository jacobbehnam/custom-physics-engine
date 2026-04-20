#include "DebugSettings.h"

namespace {
constexpr auto kDebugGroup = "debug";
constexpr auto kShowAllPathTrailsKey = "showAllPathTrails";
constexpr auto kPathTrailLengthKey = "pathTrailLength";
}

void DebugSettings::load(QSettings& settings) {
    settings.beginGroup(kDebugGroup);
    showAllPathTrails = settings.value(kShowAllPathTrailsKey, showAllPathTrails).toBool();
    pathTrailLength = settings.value(kPathTrailLengthKey, pathTrailLength).toInt();
    settings.endGroup();
}

void DebugSettings::save(QSettings& settings) const {
    settings.beginGroup(kDebugGroup);
    settings.setValue(kShowAllPathTrailsKey, showAllPathTrails);
    settings.setValue(kPathTrailLengthKey, pathTrailLength);
    settings.endGroup();
}
