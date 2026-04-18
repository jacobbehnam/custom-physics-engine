#pragma once

#include <QMainWindow>
#include <QPointer>
#include <QToolBar>
#include <QLabel>
#include <QGridLayout>
#include <QTreeWidget>
#include <QTableWidget>
#include <vector>

#include "SnapshotTableModel.h"
#include "graphics/core/SceneManager.h"

class OpenGLWindow;
class InspectorWidget;
class HierarchyWidget;
class FrameGraphWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    OpenGLWindow* getGlWindow() { return glWindow; }

protected:
    void resizeEvent(QResizeEvent* event) override;   

private slots:
    void onGLInitialized();
    void onHierarchySelectionChanged(SceneObject* previous, SceneObject* current);
    void showObjectContextMenu(const QPoint& pos, SceneObject* obj);
    void clearFrameGraph();

private:
    OpenGLWindow* glWindow;
    SceneManager* sceneManager;

    QTreeWidgetItem* previousItem = nullptr;
    QLabel* fpsLabel;

    InspectorWidget* inspector;
    HierarchyWidget* hierarchy;
    SnapshotTableModel* snapshotModel;
    QPointer<QWidget> frameGraphContainer;
    QGridLayout* frameGraphLayout = nullptr;
    std::vector<FrameGraphWidget*> frameGraphs;
    int frameGraphColumns = 0;

    void relayoutFrameGraphs();
    void setupDockWidgets();
    void setupFileMenu();
    void setupSettingMenu();
    void setupMenuBar();
    void loadAppSettings();
    void saveCameraSettings(const CameraSettings& cameraSettings);
};
