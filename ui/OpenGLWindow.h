#pragma once
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSet>
#include <QCursor>

#include "physics/PhysicsSystem.h"

class Scene;
namespace Physics { class PhysicsSystem; }

class OpenGLWindow : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    OpenGLWindow(QWidget* parent = nullptr);
    ~OpenGLWindow();

    bool isKeyPressed(int qtKey) const {
        return pressedKeys.contains(qtKey);
    }

    QPointF getMousePos() const {
        return mapFromGlobal(QCursor::pos());
    }

    QSize getFramebufferSize() const {
        return size(); // same as width()/height()
    }

    void setScene(Scene* sc) {scene = sc;}

    void setMouseCaptured(bool captured) {
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

    bool isMouseCaptured() const {
        return mouseCaptured;
    }

    bool isMouseButtonHeld(Qt::MouseButton button) const {
        if (button == Qt::LeftButton) return mouseLeftHeld;
        if (button == Qt::RightButton) return mouseRightHeld;
        return false;
    }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QTimer frameTimer;
    double lastFrame = 0;

    QSet<int> pressedKeys;
    bool mouseLeftHeld = false;
    bool mouseRightHeld = false;

    double mouseLastX = 0.0;
    double mouseLastY = 0.0;
    double mouseLastXBeforeCapture = 0.0;
    double mouseLastYBeforeCapture = 0.0;

    bool mouseCaptured = false;

    Scene* scene;
    Physics::PhysicsSystem* physicsSystem;
};