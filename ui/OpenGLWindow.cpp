#include "OpenGLWindow.h"
#include <QMouseEvent>
#include <QElapsedTimer>
#include "physics/PhysicsSystem.h"
#include "graphics/core/Scene.h"
#include <iostream>

#include "graphics/components/Gizmo.h"
#include "graphics/core/ResourceManager.h"

OpenGLWindow::OpenGLWindow(Scene* scn, QWidget* parent) : QOpenGLWidget(parent), scene(scn) {}

OpenGLWindow::~OpenGLWindow() {
    delete scene;
}

void OpenGLWindow::initializeGL() {
    initializeOpenGLFunctions();
    ResourceManager::initialize(this); // inherits from QOpenGLFunctions so can be cast to it
    glEnable(GL_DEPTH_TEST);

    // glEnable(GL_DEBUG_OUTPUT);
    // glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
    //                           GLsizei length, const GLchar* message, const void* userParam) {
    //     std::cerr << "OpenGL DEBUG: " << message << std::endl;
    // }, nullptr);

    glLineWidth(10.0f);

    lastFrame = std::chrono::steady_clock::now();
    emit glInitialized();
}

void OpenGLWindow::resizeGL(int w, int h) {
    // TODO: camera should be in SceneManager
    glViewport(0, 0, w, h);
    scene->getCamera()->setAspectRatio(static_cast<float>(w) / h);
}

void OpenGLWindow::paintGL() {
    auto currentFrame = std::chrono::steady_clock::now();
    std::chrono::duration<double> deltaDuration = currentFrame - lastFrame;
    double deltaTime = deltaDuration.count();
    lastFrame = currentFrame;

    if (simulating) {
        renderSimTime += deltaTime * 5.0f;
    }

    //sceneManager->stepPhysics(deltaTime);
    // 1) Acquire the latest batch of snapshots
    auto snaps = sceneManager->physicsSystem->fetchLatestSnapshot(renderSimTime);

    sceneManager->processHeldKeys(pressedKeys, deltaTime);

    MathUtils::Ray ray = getMouseRay();
    sceneManager->updateHoverState(ray);
    scene->draw(snaps, sceneManager->hoveredIDs, sceneManager->selectedIDs);

    calculateFPS();

    update();
}

MathUtils::Ray OpenGLWindow::getMouseRay() {
    QPointF mousePos = getMousePos();
    QSize fbSize = getFramebufferSize();

    return {
        scene->getCamera()->position,
        MathUtils::screenToWorldRayDirection(
            mousePos.x(), mousePos.y(),
            fbSize.width(), fbSize.height(),
            scene->getCamera()->getViewMatrix(), scene->getCamera()->getProjMatrix())
    };
}


void OpenGLWindow::calculateFPS() {
    static int frameCount = 0;
    static double fps = 0.0f;
    static std::chrono::steady_clock::time_point lastFpsUpdate = lastFrame;

    frameCount++;

    std::chrono::duration<double> fpsDuration = lastFrame - lastFpsUpdate;
    if (fpsDuration.count() >= 0.1) {  // Every 100 milliseconds
        fps = frameCount / fpsDuration.count();
        frameCount = 0;
        lastFpsUpdate = lastFrame;
        emit fpsUpdated(fps);
    }
}


void OpenGLWindow::keyPressEvent(QKeyEvent* event) {
    simulating = true;
    pressedKeys.insert(event->key());
}

void OpenGLWindow::keyReleaseEvent(QKeyEvent* event) {
    pressedKeys.remove(event->key());
}

void OpenGLWindow::mousePressEvent(QMouseEvent* event) {
    pressedMouseButtons.insert(event->button());
    setFocus();
    if (sceneManager)
        sceneManager->handleMouseButton(event->button(), event->type(), event->modifiers()); // adapt as needed
    update();
}

void OpenGLWindow::mouseReleaseEvent(QMouseEvent* event) {
    pressedMouseButtons.remove(event->button());

    if (sceneManager)
        sceneManager->handleMouseButton(event->button(), QEvent::MouseButtonRelease, event->modifiers());
    update();
}

void OpenGLWindow::setMouseCaptured(bool captured) {
    mouseCaptured = captured;
    if (captured) {
        mouseLastPosBeforeCapture = QCursor::pos();
        setCursor(Qt::BlankCursor);
    } else {
        setCursor(Qt::ArrowCursor);
        QCursor::setPos(mouseLastPosBeforeCapture);
    }
}

void OpenGLWindow::handleRawMouseDelta(int dx, int dy) {
    if (mouseCaptured) {
        QCursor::setPos(mouseLastPosBeforeCapture);
        scene->getCamera()->processMouseMovement(dx, -dy);
    }
}
