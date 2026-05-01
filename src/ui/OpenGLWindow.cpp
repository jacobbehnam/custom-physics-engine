#include "ui/OpenGLWindow.h"
#include <QMouseEvent>
#include <QElapsedTimer>
#include "physics/PhysicsSystem.h"
#include "graphics/core/Scene.h"
#include "graphics/core/SceneManager.h"

#include "graphics/components/Gizmo.h"
#include "graphics/core/ResourceManager.h"
#include "graphics/core/SceneObject.h"
#include "ui/AppSettings.h"
#include "ui/settings/DebugSettings.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

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
        renderSimTime += deltaTime * simSpeed;
    }

    //sceneManager->stepPhysics(deltaTime);
    // 1) Acquire the latest batch of snapshots
    auto snaps = sceneManager->physicsSystem->fetchLatestSnapshot(renderSimTime);

    sceneManager->processHeldKeys(pressedKeys, deltaTime);

    Math::Ray ray = getMouseRay();
    sceneManager->updateHoverState(ray);
    scene->draw(snaps, sceneManager->hoveredIDs, sceneManager->selectedIDs);
    updateObjectLabels();

    calculateFPS();

    update();
}

Math::Ray OpenGLWindow::getMouseRay() {
    QPointF mousePos = getMousePos();
    QSize fbSize = getFramebufferSize();

    return {
        scene->getCamera()->position,
        Math::screenToWorldRayDirection(
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
        sceneManager->handleMouseButton(event->button(), event->type(), event->modifiers());
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

void OpenGLWindow::hideObjectLabels() {
    for (QPushButton* button : objectLabelButtons) {
        if (button) button->hide();
    }
}

void OpenGLWindow::updateObjectLabels() {
    if (!scene || !sceneManager || mouseCaptured) {
        hideObjectLabels();
        return;
    }

    auto& dbg = AppSettings::getInstance().getGroup<DebugSettings>();
    if (!dbg.showObjectLabels) {
        hideObjectLabels();
        return;
    }

    const auto objects = sceneManager->getObjects();
    while (objectLabelButtons.size() < objects.size()) {
        auto* button = new QPushButton(this);
        button->setFocusPolicy(Qt::NoFocus);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(
            "QPushButton {"
            " background: rgba(18, 22, 28, 190);"
            " border: 1px solid rgba(160, 210, 255, 210);"
            " color: white;"
            " padding: 2px 6px;"
            " font-size: 11px;"
            "}"
            "QPushButton:hover {"
            " background: rgba(35, 85, 130, 220);"
            " border-color: rgba(220, 240, 255, 240);"
            "}"
        );
        connect(button, &QPushButton::clicked, this, [this, button]() {
            if (!sceneManager) return;
            const uint32_t objectID = button->property("objectID").toUInt();
            if (SceneObject* obj = sceneManager->getObjectByID(objectID)) {
                sceneManager->focusObject(obj);
                setFocus();
            }
        });
        objectLabelButtons.push_back(button);
    }

    const glm::mat4 view = scene->getCamera()->getViewMatrix();
    const glm::mat4 proj = scene->getCamera()->getProjMatrix();
    const float w = static_cast<float>(width());
    const float h = static_cast<float>(height());

    for (size_t i = 0; i < objectLabelButtons.size(); ++i) {
        QPushButton* button = objectLabelButtons[i];
        if (i >= objects.size()) {
            button->hide();
            continue;
        }

        SceneObject* obj = objects[i];
        const glm::vec4 clip = proj * view * glm::vec4(obj->getPosition(), 1.0f);
        if (!std::isfinite(clip.x) || !std::isfinite(clip.y) || !std::isfinite(clip.z) || !std::isfinite(clip.w) || clip.w <= 0.0f) {
            button->hide();
            continue;
        }

        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (!std::isfinite(ndc.x) || !std::isfinite(ndc.y) || !std::isfinite(ndc.z) ||
            ndc.x < -1.1f || ndc.x > 1.1f || ndc.y < -1.1f || ndc.y > 1.1f || ndc.z < -1.0f || ndc.z > 1.0f) {
            button->hide();
            continue;
        }

        button->setText(QString::fromStdString(obj->getName()));
        button->setProperty("objectID", obj->getObjectID());
        button->adjustSize();

        const int x = static_cast<int>((ndc.x * 0.5f + 0.5f) * w) - button->width() / 2;
        const int y = static_cast<int>((1.0f - (ndc.y * 0.5f + 0.5f)) * h) - button->height() - 8;
        button->move(std::clamp(x, 0, std::max(0, width() - button->width())),
                     std::clamp(y, 0, std::max(0, height() - button->height())));
        button->show();
        button->raise();
    }
}
