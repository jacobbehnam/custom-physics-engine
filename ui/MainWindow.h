#pragma once

#include <QMainWindow>
#include <QToolBar>
#include <QLabel>
#include <QTreeWidget>
#include <QTableWidget>

#include "SnapshotTableModel.h"
#include "graphics/core/SceneManager.h"

class OpenGLWindow;
class InspectorWidget;
class HierarchyWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    OpenGLWindow* getGlWindow() { return glWindow; }

private slots:
    void onGLInitialized();
    void onHierarchySelectionChanged(SceneObject* previous, SceneObject* current);
    void showObjectContextMenu(const QPoint& pos, SceneObject* obj);

private:
    OpenGLWindow* glWindow;
    SceneManager* sceneManager;

    QTreeWidgetItem* previousItem = nullptr;
    QLabel* fpsLabel;

    InspectorWidget* inspector;
    HierarchyWidget* hierarchy;
    SnapshotTableModel* snapshotModel;

    void setupDockWidgets();
    void setupMenuBar();
};