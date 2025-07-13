#include "OpenGLWindow.h"
#include <QMouseEvent>
#include <QElapsedTimer>
#include "physics/PhysicsSystem.h"
#include "graphics/core/Scene.h"
#include <iostream>

OpenGLWindow::OpenGLWindow(QWidget* parent) : QOpenGLWidget(parent), scene(nullptr), physicsSystem(new Physics::PhysicsSystem) {

}

OpenGLWindow::~OpenGLWindow() {
    delete scene;
    delete physicsSystem;
}

void OpenGLWindow::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_DEBUG_OUTPUT);
    // glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
    //                           GLsizei length, const GLchar* message, const void* userParam) {
    //     std::cerr << "OpenGL DEBUG: " << message << std::endl;
    // }, nullptr);

    glLineWidth(10.0f);

    scene = new Scene(this, physicsSystem);
    lastFrame = std::chrono::steady_clock::now();
}

void OpenGLWindow::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void OpenGLWindow::paintGL() {
    auto currentFrame = std::chrono::steady_clock::now();
    std::chrono::duration<double> deltaDuration = currentFrame - lastFrame;
    double deltaTime = deltaDuration.count();
    lastFrame = currentFrame;

    physicsSystem->step(deltaTime);
    scene->processInput(deltaTime);
    scene->draw();

    // FPS tracking
    static int frameCount = 0;
    static std::chrono::steady_clock::time_point lastFpsUpdate = currentFrame;

    frameCount++;

    std::chrono::duration<double> fpsDuration = currentFrame - lastFpsUpdate;
    if (fpsDuration.count() >= 1.0) {  // Every second
        double fps = frameCount / fpsDuration.count();
        emit fpsUpdated(fps);
        frameCount = 0;
        lastFpsUpdate = currentFrame;
    }

    update();
}

void OpenGLWindow::keyPressEvent(QKeyEvent* event) {
    pressedKeys.insert(event->key());
}

void OpenGLWindow::keyReleaseEvent(QKeyEvent* event) {
    pressedKeys.remove(event->key());
}

void OpenGLWindow::mousePressEvent(QMouseEvent* event) {
    if (scene)
        scene->handleMouseButton(event->button(), event->type(), 0); // adapt as needed
    update();
}

void OpenGLWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) mouseLeftHeld = false;
    if (event->button() == Qt::RightButton) mouseRightHeld = false;

    if (scene)
        scene->handleMouseButton(event->button(), QEvent::MouseButtonRelease, event->modifiers());
    update();
}

void OpenGLWindow::setMouseCaptured(bool captured) {
    if (captured && !mouseCaptured) {
        mouseCaptured = true;
        QPoint globalPos = QCursor::pos();
        mouseLastXBeforeCapture = globalPos.x();
        mouseLastYBeforeCapture = globalPos.y();
        setCursor(Qt::BlankCursor);
        grabMouse();
    } else if (!captured && mouseCaptured) {
        mouseCaptured = false;
        releaseMouse();
        setCursor(Qt::ArrowCursor);
        QCursor::setPos(mapToGlobal(QPoint(mouseLastXBeforeCapture, mouseLastYBeforeCapture)));
    }
}

bool OpenGLWindow::isMouseButtonHeld(Qt::MouseButton button) const {
    if (button == Qt::LeftButton) return mouseLeftHeld;
    if (button == Qt::RightButton) return mouseRightHeld;
    return false;
}
