#include "graphics/presets/ScenePresets.h"

#include "graphics/presets/AstronomyPresets.h"

namespace {
constexpr ScenePresets::PresetDescriptor kPresets[] = {
    {
        "Astronomical Systems",
        "Real Solar System",
        "Real-scale Sun, planets, and Moon generated from orbital elements.",
        &ScenePresets::Astronomy::createRealSolarSystem
    },
};
}

namespace ScenePresets {

std::span<const PresetDescriptor> all() {
    return kPresets;
}

}
