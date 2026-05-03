#pragma once
#include "ISettingsGroup.h"

struct GraphicsSettings : public ISettingsGroup {
    static constexpr float kMinRayTraceResolutionScale = 0.25f;
    static constexpr float kMaxRayTraceResolutionScale = 1.0f;

    bool useRayTraced = false;
    /// Internal RT resolution as fraction of framebuffer (0.25-1). Lower = much faster; upscaled to full window.
    float rayTraceResolutionScale = kMaxRayTraceResolutionScale;
    bool enableGlobalLight = true;

    void load(QSettings& settings) override;
    void save(QSettings& settings) const override;
};
