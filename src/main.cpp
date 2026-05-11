#include <QApplication>
#include <ui/MainWindow.h>
#include <cstdint>
#include <cstdlib>
#include <string_view>

#include "ui/OpenGLWindow.h"
#include "ui/RawInputFilter.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)
#define PHYSICS_ENGINE_GPU_EXPORT __declspec(dllexport)
#else
#define PHYSICS_ENGINE_GPU_EXPORT __attribute__((visibility("default")))
#endif
extern "C" {
    PHYSICS_ENGINE_GPU_EXPORT uint32_t NvOptimusEnablement = 1;
    PHYSICS_ENGINE_GPU_EXPORT int AmdPowerXpressRequestHighPerformance = 1;
}
#undef PHYSICS_ENGINE_GPU_EXPORT
#endif

namespace {

void applyDiscreteGpuHints() {
#if defined(__linux__)
    const char* preferDiscrete = std::getenv("PHYSICS_ENGINE_PREFER_DISCRETE_GPU");
    if (!preferDiscrete || std::string_view(preferDiscrete) != "1") {
        return;
    }

    setenv("DRI_PRIME", "1", 0);
    setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 0);
#endif
}

} // namespace

int main(int argc, char** argv) {
    applyDiscreteGpuHints();

    QApplication app(argc, argv);
    app.setOrganizationName("PhysicsEngine");
    app.setApplicationName("PhysicsEngine");

    MainWindow mainWindow;
    mainWindow.resize(800, 600);
    mainWindow.show();

    // input gets handled by RawInputFilter before Qt handles it
    RawInputFilter filter([&](int dx, int dy) {
        mainWindow.getGlWindow()->handleRawMouseDelta(dx, dy);
    });
    app.installNativeEventFilter(&filter);

    return QApplication::exec();
}
