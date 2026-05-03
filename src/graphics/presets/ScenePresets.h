#pragma once

#include <span>

class SceneManager;

namespace ScenePresets {

using GenerateFn = void (*)(SceneManager&);

struct PresetDescriptor {
    const char* category;
    const char* name;
    const char* description;
    GenerateFn generate;
};

std::span<const PresetDescriptor> all();

}
