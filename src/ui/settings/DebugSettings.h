#pragma once
#include "ISettingsGroup.h"

struct DebugSettings : public ISettingsGroup {
    bool showAllPathTrails = true;
    float pathTrailTime = 2.0f;
    bool showForces = true;
    bool showColliders = true;

    void load(QSettings& settings) override;
    void save(QSettings& settings) const override;
};
