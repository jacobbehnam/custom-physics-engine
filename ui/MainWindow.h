#pragma once

#include <QMainWindow>
#include <QToolBar>
#include <QLabel>

#include "graphics/core/SceneManager.h"

class OpenGLWindow;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    OpenGLWindow* getGlWindow() { return glWindow; }

private:
    OpenGLWindow* glWindow;
    SceneManager* sceneManager;

    QLabel* fpsLabel;

    void setupDockWidgets();
};