#include "OpenGLWindow.h"
#include <QMouseEvent>
#include <QElapsedTimer>
#include "physics/PhysicsSystem.h"
#include "graphics/core/Scene.h"
#include <iostream>

OpenGLWindow::OpenGLWindow(QWidget* parent)
    : QOpenGLWidget(parent),
      scene(nullptr),
        physicsSystem(new Physics::PhysicsSystem)
{
    frameTimer.setInterval(16); // ~60 FPS
    connect(&frameTimer, &QTimer::timeout, this, QOverload<>::of(&OpenGLWindow::update));
    frameTimer.start();
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
    lastFrame = QElapsedTimer().elapsed() / 1000.0;
}

void OpenGLWindow::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void OpenGLWindow::paintGL() {
    double currentFrame = QElapsedTimer().elapsed() / 1000.0;
    double deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    physicsSystem->step(deltaTime);
    scene->processInput(deltaTime);
    scene->draw();
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

    if (scene) scene->handleMouseButton(event->button(), QEvent::MouseButtonRelease, event->modifiers());
    update();
}