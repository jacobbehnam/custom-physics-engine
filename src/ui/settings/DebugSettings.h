#pragma once
#include "ISettingsGroup.h"

struct DebugSettings : public ISettingsGroup {
    bool showAllPathTrails = true;
    float pathTrailTime = 2.0f;
    bool showForces = true;
    bool showColliders = true;
    bool useRayTraced = false;
    bool rayTraceRequireGpu = false;
    /// Internal RT resolution as fraction of framebuffer (0.25–1). Lower = much faster; upscaled to full window.
    float rayTraceResolutionScale = 1.0f;

    void load(QSettings& settings) override;
    void save(QSettings& settings) const override;
};
