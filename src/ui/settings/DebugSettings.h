#pragma once
#include "ISettingsGroup.h"

struct DebugSettings : public ISettingsGroup {
    bool showAllPathTrails = true;
    int pathTrailLength = 200;

    void load(QSettings& settings) override;
    void save(QSettings& settings) const override;
};
