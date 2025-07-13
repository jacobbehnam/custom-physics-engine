#pragma once
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSet>
#include <QCursor>
#include <chrono>

#include "physics/PhysicsSystem.h"

class Scene;
namespace Physics { class PhysicsSystem; }

class OpenGLWindow : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT
signals:
    void fpsUpdated(double fps);

public:
    explicit OpenGLWindow(QWidget* parent = nullptr);
    ~OpenGLWindow();

    bool isKeyPressed(int qtKey) const { return pressedKeys.contains(qtKey); }

    QPointF getMousePos() const { return mapFromGlobal(QCursor::pos()); }

    QSize getFramebufferSize() const { return size(); }

    void setScene(Scene* sc) { scene = sc; }

    void setMouseCaptured(bool captured);

    bool isMouseCaptured() const { return mouseCaptured; }

    bool isMouseButtonHeld(Qt::MouseButton button) const;

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
    bool mouseLeftHeld = false;
    bool mouseRightHeld = false;

    double mouseLastX = 0.0f;
    double mouseLastY = 0.0f;
    double mouseLastXBeforeCapture = 0.0f;
    double mouseLastYBeforeCapture = 0.0f;

    bool mouseCaptured = false;

    Scene* scene;
    Physics::PhysicsSystem* physicsSystem;
};