#include "ui/OpenGLWindow.h"
#include <QMouseEvent>
#include <QElapsedTimer>
#include <QRect>
#include "physics/PhysicsSystem.h"
#include "graphics/core/Scene.h"
#include "graphics/core/SceneManager.h"
#include "ui/AppSettings.h"
#include "ui/settings/DebugSettings.h"
#include "ui/settings/GraphicsSettings.h"
#include "graphics/raytrace/SceneRayTracer.h"
#include "graphics/core/ResourceManager.h"
#include "graphics/core/SceneObject.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr float  kDefaultGlLineWidth            = 10.0f;
constexpr double kFpsUpdateIntervalSeconds      = 0.1;
constexpr int    kWheelDegreesPerStep           = 15;
constexpr int    kWheelDeltaDivisor             = 8;
constexpr int    kLabelGridCellPx               = 32;
constexpr float  kClipWMin                      = 0.000001f;
constexpr float  kOffscreenDirectionEpsilonSq   = 1.0e-6f;
constexpr float  kEdgeClampX                    = 0.98f;
constexpr float  kEdgeClampY                    = 0.95f;
constexpr int    kLabelPaddingPx                = 2;
constexpr int    kLabelOffsetAboveObjectPx      = 8;
constexpr int    kLabelStepYPx                  = 4;
constexpr int    kLabelMinStepXPx               = 36;
constexpr int    kLabelSearchRings              = 12;
}

QSize OpenGLWindow::getFramebufferSize() const {
    const qreal dpr = devicePixelRatioF();
    return QSize(
        static_cast<int>(std::lround(static_cast<qreal>(width()) * dpr)),
        static_cast<int>(std::lround(static_cast<qreal>(height()) * dpr)));
}

OpenGLWindow::OpenGLWindow(QWidget* parent) : QOpenGLWidget(parent) {}

void OpenGLWindow::initializeGL() {
    initializeOpenGLFunctions();
    ResourceManager::initialize(this); // inherits from QOpenGLFunctions so can be cast to it
    glEnable(GL_DEPTH_TEST);
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glClearDepth(0.0);
    glDepthFunc(GL_GREATER);

    // glEnable(GL_DEBUG_OUTPUT);
    // glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
    //                           GLsizei length, const GLchar* message, const void* userParam) {
    //     std::cerr << "OpenGL DEBUG: " << message << std::endl;
    // }, nullptr);

    glLineWidth(kDefaultGlLineWidth);

    lastFrame = std::chrono::steady_clock::now();
    emit glInitialized();
}

void OpenGLWindow::resizeGL(int w, int h) {
    if (w <= 0 || h <= 0 || !scene) {
        return;
    }
    // Match device-pixel framebuffer (same as getFramebufferSize + Qt paintGL viewport).
    const qreal dpr = devicePixelRatioF();
    const int pw = static_cast<int>(std::lround(static_cast<qreal>(w) * dpr));
    const int ph = static_cast<int>(std::lround(static_cast<qreal>(h) * dpr));
    glViewport(0, 0, pw, ph);
    scene->getCamera()->setAspectRatio(static_cast<float>(pw) / static_cast<float>(ph));
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
    scene->getCamera()->update();

    Math::Ray ray = getMouseRay();
    sceneManager->updateHoverState(ray);
    auto& visRt = AppSettings::getInstance().getGroup<GraphicsSettings>();
    if (visRt.useRayTraced && sceneManager->getRayTracer() && sceneManager->getRayTracer()->isUsable()) {
        scene->applyPhysicsSnapshots(snaps);
        float sc = visRt.rayTraceResolutionScale;
        if (sc < GraphicsSettings::kMinRayTraceResolutionScale) {
            sc = GraphicsSettings::kMinRayTraceResolutionScale;
        }
        if (sc > GraphicsSettings::kMaxRayTraceResolutionScale) {
            sc = GraphicsSettings::kMaxRayTraceResolutionScale;
        }
        const QSize fb = getFramebufferSize();
        sceneManager->getRayTracer()->render(fb.width(), fb.height(), scene->getCamera(), snaps, sc);
        glClear(GL_DEPTH_BUFFER_BIT);
        scene->drawCustomDrawables(snaps, sceneManager->hoveredIDs, sceneManager->selectedIDs);
    } else {
        scene->draw(snaps, sceneManager->hoveredIDs, sceneManager->selectedIDs);
    }
    updateObjectLabels();

    calculateFPS();

    update();
}

Math::Ray OpenGLWindow::getMouseRay() {
    QPointF mousePos = getMousePos();
    const qreal dpr = devicePixelRatioF();
    const double mxd = mousePos.x() * dpr;
    const double myd = mousePos.y() * dpr;
    const QSize fbSize = getFramebufferSize();
    Camera* camera = scene->getCamera();

    return {
        camera->position,
        Math::screenToWorldRayDirection(
            mxd, myd,
            fbSize.width(), fbSize.height(),
            camera->front, camera->right, camera->up, camera->fov)
    };
}


void OpenGLWindow::calculateFPS() {
    static int frameCount = 0;
    static double fps = 0.0f;
    static std::chrono::steady_clock::time_point lastFpsUpdate = lastFrame;

    frameCount++;

    std::chrono::duration<double> fpsDuration = lastFrame - lastFpsUpdate;
    if (fpsDuration.count() >= kFpsUpdateIntervalSeconds) {
        fps = frameCount / fpsDuration.count();
        frameCount = 0;
        lastFpsUpdate = lastFrame;
        emit fpsUpdated(fps);
    }
}


void OpenGLWindow::keyPressEvent(QKeyEvent* event) {
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

void OpenGLWindow::wheelEvent(QWheelEvent* event) {
    if (!scene) return;

    const QPoint numDegrees = event->angleDelta() / kWheelDeltaDivisor;
    const float wheelSteps = static_cast<float>(numDegrees.y()) / static_cast<float>(kWheelDegreesPerStep);
    scene->getCamera()->processScroll(wheelSteps);
    event->accept();
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
    if (!scene || !sceneManager) {
        hideObjectLabels();
        return;
    }

    auto& dbg = AppSettings::getInstance().getGroup<DebugSettings>();
    if (!dbg.showObjectLabels) {
        hideObjectLabels();
        return;
    }

    const auto& objects = sceneManager->getObjects();
    while (objectLabelButtons.size() < objects.size()) {
        auto* button = new QPushButton(this);
        button->setFocusPolicy(Qt::NoFocus);
        button->setCursor(Qt::PointingHandCursor);
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

    const glm::vec3 renderOrigin = SceneObject::getRenderOrigin();
    const bool useFloatingOrigin = std::max({std::abs(renderOrigin.x), std::abs(renderOrigin.y), std::abs(renderOrigin.z)}) > 0.0f;
    const glm::mat4 view = useFloatingOrigin ? scene->getCamera()->getRenderViewMatrix() : scene->getCamera()->getViewMatrix();
    const glm::mat4 proj = scene->getCamera()->getProjMatrix();
    const float w = static_cast<float>(width());
    const float h = static_cast<float>(height());

    const int labelGridCols = std::max(1, (width() + kLabelGridCellPx - 1) / kLabelGridCellPx);
    const int labelGridRows = std::max(1, (height() + kLabelGridCellPx - 1) / kLabelGridCellPx);
    std::vector<unsigned char> occupiedLabelCells(static_cast<size_t>(labelGridCols * labelGridRows), 0);

    auto visitLabelCells = [&](const QRect& rect, auto&& visitor) {
        const int left = std::clamp(rect.left() / kLabelGridCellPx, 0, labelGridCols - 1);
        const int right = std::clamp(rect.right() / kLabelGridCellPx, 0, labelGridCols - 1);
        const int top = std::clamp(rect.top() / kLabelGridCellPx, 0, labelGridRows - 1);
        const int bottom = std::clamp(rect.bottom() / kLabelGridCellPx, 0, labelGridRows - 1);

        for (int yCell = top; yCell <= bottom; ++yCell) {
            for (int xCell = left; xCell <= right; ++xCell) {
                if (visitor(xCell, yCell))
                    return true;
            }
        }
        return false;
    };

    auto overlapsOccupiedLabel = [&](const QRect& rect) {
        const QRect padded = rect.adjusted(-kLabelPaddingPx, -kLabelPaddingPx, kLabelPaddingPx, kLabelPaddingPx);
        return visitLabelCells(padded, [&](int xCell, int yCell) {
            return occupiedLabelCells[static_cast<size_t>(yCell * labelGridCols + xCell)] != 0;
        });
    };

    auto occupyLabel = [&](const QRect& rect) {
        const QRect padded = rect.adjusted(-kLabelPaddingPx, -kLabelPaddingPx, kLabelPaddingPx, kLabelPaddingPx);
        visitLabelCells(padded, [&](int xCell, int yCell) {
            occupiedLabelCells[static_cast<size_t>(yCell * labelGridCols + xCell)] = 1;
            return false;
        });
    };

    for (size_t i = 0; i < objectLabelButtons.size(); ++i) {
        QPushButton* button = objectLabelButtons[i];
        if (i >= objects.size()) {
            button->hide();
            continue;
        }

        SceneObject* obj = objects[i].get();
        const glm::vec3 objectPosition = glm::vec3(obj->getModelMatrix()[3]);
        const glm::vec3 labelPosition = useFloatingOrigin ? objectPosition - renderOrigin : objectPosition;
        glm::vec4 clip = proj * view * glm::vec4(labelPosition, 1.0f);
        if (!std::isfinite(clip.x) || !std::isfinite(clip.y) || !std::isfinite(clip.z) || !std::isfinite(clip.w)) {
            button->hide();
            continue;
        }

        const bool behindCamera = clip.w < 0.0f;
        if (behindCamera) {
            clip.x = -clip.x;
            clip.y = -clip.y;
            clip.w = -clip.w;
        }

        glm::vec3 ndc = glm::vec3(clip) / std::max(clip.w, kClipWMin);
        if (!std::isfinite(ndc.x) || !std::isfinite(ndc.y) || !std::isfinite(ndc.z)) {
            button->hide();
            continue;
        }

        const bool outsideNdc = ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f;
        const bool offscreen = behindCamera || outsideNdc;
        if (offscreen) {
            const glm::vec3 toObject = objectPosition - scene->getCamera()->position;
            const float cameraX = glm::dot(toObject, scene->getCamera()->right);
            const float cameraY = glm::dot(toObject, scene->getCamera()->up);
            const float cameraZ = glm::dot(toObject, scene->getCamera()->front);
            glm::vec2 edgeDir(
                cameraZ > kClipWMin ? cameraX / cameraZ : cameraX,
                cameraZ > kClipWMin ? cameraY / cameraZ : cameraY
            );
            if (glm::dot(edgeDir, edgeDir) < kOffscreenDirectionEpsilonSq) {
                edgeDir = glm::vec2(1.0f, 0.0f);
            }

            const float scaleX = edgeDir.x != 0.0f ? kEdgeClampX / std::abs(edgeDir.x) : std::numeric_limits<float>::infinity();
            const float scaleY = edgeDir.y != 0.0f ? kEdgeClampY / std::abs(edgeDir.y) : std::numeric_limits<float>::infinity();
            const float edgeScale = std::min(scaleX, scaleY);
            ndc.x = edgeDir.x * edgeScale;
            ndc.y = edgeDir.y * edgeScale;
        } else {
            ndc.x = std::clamp(ndc.x, -kEdgeClampX, kEdgeClampX);
            ndc.y = std::clamp(ndc.y, -kEdgeClampY, kEdgeClampY);
        }

        bool metricsDirty = false;
        QString labelText = QString::fromStdString(obj->getName());
        if (offscreen) {
            if (std::abs(ndc.x) > std::abs(ndc.y)) {
                labelText = ndc.x < 0.0f
                    ? "← " + labelText
                    : labelText + " →";
            } else {
                labelText = ndc.y < 0.0f
                    ? labelText + " ↓"
                    : "↑ " + labelText;
            }
        }
        if (button->text() != labelText) {
            button->setText(labelText);
            metricsDirty = true;
        }
        button->setProperty("objectID", obj->getObjectID());
        const QVariant oldOffscreen = button->property("offscreen");
        if (!oldOffscreen.isValid() || oldOffscreen.toBool() != offscreen) {
            button->setProperty("offscreen", offscreen);
            button->setStyleSheet(offscreen
                ? "QPushButton {"
                " background: rgba(20, 22, 24, 120);"
                " border: 1px dashed rgba(20, 22, 24, 120);"
                " color: rgba(210, 210, 210, 170);"
                " padding: 1px 5px;"
                " font-size: 12px;"
                " border-radius: 2px;"
                " min-height: 0px;"
                "}"
                "QPushButton:hover {"
                " background: rgba(35, 38, 42, 170);"
                " border-color: rgba(220, 220, 220, 180);"
                " color: rgba(235, 235, 235, 220);"
                "}"

                : "QPushButton {"
                " background: rgba(28, 32, 35, 190);"
                " border: 1px solid rgba(115, 130, 135, 150);"
                " color: rgba(235, 235, 235, 235);"
                " padding: 1px 5px;"
                " font-size: 12px;"
                " border-radius: 2px;"
                " min-height: 0px;"
                "}"
                "QPushButton:hover {"
                " background: rgba(44, 50, 54, 225);"
                " border-color: rgba(150, 170, 175, 190);"
                "}"
            );
            metricsDirty = true;
        }
        const QVariant metricsReady = button->property("metricsReady");
        if (metricsDirty || !metricsReady.isValid() || !metricsReady.toBool()) {
            button->adjustSize();
            button->setProperty("metricsReady", true);
        }

        const int maxX = std::max(0, width() - button->width());
        const int maxY = std::max(0, height() - button->height());
        const int baseX = std::clamp(static_cast<int>((ndc.x * 0.5f + 0.5f) * w) - button->width() / 2, 0, maxX);
        const int baseY = std::clamp(static_cast<int>((1.0f - (ndc.y * 0.5f + 0.5f)) * h) - button->height() - kLabelOffsetAboveObjectPx, 0, maxY);
        const int stepY = button->height() + kLabelStepYPx;
        const int stepX = std::max(button->width() / 2, kLabelMinStepXPx);
        int bestX = baseX;
        int bestY = baseY;
        int bestScore = std::numeric_limits<int>::max();

        for (int ring = 0; ring <= kLabelSearchRings; ++ring) {
            for (int dySign : {1, -1}) {
                const int dy = ring == 0 ? 0 : dySign * ring * stepY;
                for (int dxSign : {0, 1, -1}) {
                    const int dx = dxSign * ring * stepX;
                    const int candidateX = std::clamp(baseX + dx, 0, maxX);
                    const int candidateY = std::clamp(baseY + dy, 0, maxY);
                    const QRect candidate(candidateX, candidateY, button->width(), button->height());
                    if (overlapsOccupiedLabel(candidate)) continue;

                    const int score = (candidateX - baseX) * (candidateX - baseX) + (candidateY - baseY) * (candidateY - baseY);
                    if (score < bestScore) {
                        bestScore = score;
                        bestX = candidateX;
                        bestY = candidateY;
                    }
                }
            }
            if (bestScore != std::numeric_limits<int>::max()) break;
        }

        const QRect placed(bestX, bestY, button->width(), button->height());
        occupyLabel(placed);
        button->move(bestX, bestY);
        button->show();
        button->raise();
    }
}
