#pragma once
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSet>
#include <QCursor>
#include <chrono>
#include <graphics/utils/MathUtils.h>

#include "graphics/core/SceneManager.h"
#include "physics/PhysicsSystem.h"

class Scene;
namespace Physics { class PhysicsSystem; }

class OpenGLWindow : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT
signals:
    void fpsUpdated(double fps);
    void glInitialized();

public:
    explicit OpenGLWindow(Scene* scene, QWidget* parent = nullptr);
    ~OpenGLWindow();

    bool isKeyPressed(int qtKey) const { return pressedKeys.contains(qtKey); }

    QPointF getMousePos() const { return mapFromGlobal(QCursor::pos()); }

    QSize getFramebufferSize() const { return size(); }

    void setScene(Scene* sc) { scene = sc; }
    void setSceneManager(SceneManager* scm) { sceneManager = scm; }

    void setMouseCaptured(bool captured);

    bool isMouseCaptured() const { return mouseCaptured; }

    bool isMouseButtonHeld(Qt::MouseButton button) const { return pressedMouseButtons.contains(button); };

    void handleRawMouseDelta(int dx, int dy);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    std::chrono::steady_clock::time_point lastFrame;

    QSet<int> pressedKeys;
    QSet<Qt::MouseButton> pressedMouseButtons;
    bool mouseLeftHeld = false;
    bool mouseRightHeld = false;

    QPoint mouseLastPosBeforeCapture;

    bool firstMouse = false;
    bool mouseCaptured = false;

    Scene* scene;
    SceneManager* sceneManager;

    double calculateFPS();
    MathUtils::Ray getMouseRay();
};