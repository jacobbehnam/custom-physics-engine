#pragma once

#include <QMainWindow>
#include <QToolBar>
#include <QLabel>
#include <QTreeWidget>

#include "graphics/core/SceneManager.h"

class OpenGLWindow;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    OpenGLWindow* getGlWindow() { return glWindow; }

private slots:
    void onGLInitialized();
    void onHierarchyItemSelected();
    void changeHierarchyItemSelected(SceneObject* obj);

private:
    Scene* scene;
    OpenGLWindow* glWindow;
    SceneManager* sceneManager;

    QTreeWidget* hierarchyTree;
    QTreeWidgetItem* previousItem = nullptr;
    QLabel* fpsLabel;

    void setupDockWidgets();
};