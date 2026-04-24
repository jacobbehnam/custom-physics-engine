#pragma once
#include "ISettingsGroup.h"

struct GraphicsSettings : public ISettingsGroup {
    bool useRayTraced = false;
    bool rayTraceRequireGpu = true;
    /// Internal RT resolution as fraction of framebuffer (0.25–1). Lower = much faster; upscaled to full window.
    float rayTraceResolutionScale = 1.0f;
    bool enableSun = true;

    void load(QSettings& settings) override;
    void save(QSettings& settings) const override;
};
