#pragma once
#include "ISettingsGroup.h"

struct GraphicsSettings : public ISettingsGroup {
    static constexpr float kMinRayTraceResolutionScale = 0.25f;
    static constexpr float kMaxRayTraceResolutionScale = 1.0f;
    static constexpr float kMinRayTraceExposure = 0.01f;
    static constexpr float kMaxRayTraceExposure = 100.0f;

    bool useRayTraced = false;
    /// Internal RT resolution as fraction of framebuffer (0.25-1). Lower = much faster; upscaled to full window.
    float rayTraceResolutionScale = kMaxRayTraceResolutionScale;
    float rayTraceExposure = 1.0f;
    bool enableGlobalLight = true;

    void load(QSettings& settings) override;
    void save(QSettings& settings) const override;
};
