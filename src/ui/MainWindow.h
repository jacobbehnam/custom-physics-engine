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
class FrameGraphPanel;

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
    QLabel* cameraPositionLabel;
    QLabel* selectedObjectLabel;
    QLabel* selectedObjectPositionLabel;
    QLabel* selectedObjectDistanceLabel;
    QLabel* simulationStateLabel;
    QLabel* renderClockStateLabel;
    SceneObject* selectedInfoObject = nullptr;

    InspectorWidget* inspector;
    HierarchyWidget* hierarchy;
    SnapshotTableModel* snapshotModel;
    FrameGraphPanel* frameGraphPanel;

    void setupDockWidgets();
    void setupFileMenu();
    void setupSettingMenu();
    void setupMenuBar();
    void loadAppSettings();
    void updateStatusPanel();
};
