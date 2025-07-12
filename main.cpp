#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <graphics/components/Mesh.h>
#include <graphics/core/SceneObject.h>
#include <graphics/components/Shader.h>
#include "graphics/components/TranslateHandle.h"
#include <graphics/core/Scene.h>
#include "graphics/core/ResourceManager.h"
#include "physics/PhysicsSystem.h"
#include <QApplication>
#include <ui/OpenGLWindow.h>
#include <graphics/core/Scene.h>
#include <physics/PhysicsSystem.h>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // Create physics system
    Physics::PhysicsSystem physicsSystem;

    // Create your OpenGL widget/window
    OpenGLWindow window;

    // Setup OpenGL format (request OpenGL 4.5 core profile)
    QSurfaceFormat format;
    format.setVersion(4, 5);  // or 4,6 if available
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    // Show the window
    window.resize(800, 600);
    window.show();

    // Create the scene, passing the OpenGLWindow and physics system
    Scene scene(&window, &physicsSystem);

    // Give the OpenGLWindow access to the scene (optional, but useful)
    window.setScene(&scene);

    // Timing variables
    QElapsedTimer timer;
    timer.start();
    double lastFrame = timer.elapsed() / 1000.0;

    // Use a QTimer to trigger updates at ~60 FPS
    QTimer updateTimer;
    QObject::connect(&updateTimer, &QTimer::timeout, [&]() {
        double currentFrame = timer.elapsed() / 1000.0;
        double deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        physicsSystem.step(deltaTime);

        scene.processInput(static_cast<float>(deltaTime));
        window.update(); // Triggers paintGL()
    });
    updateTimer.start(16); // ~60 FPS (16 ms interval)

    // Run Qt application event loop
    return app.exec();
}