#pragma once
#include <QWidget>
#include <QTimer>
#include <QVBoxLayout>

class QTabWidget;
class SceneManager;
class SceneObject;

class TransformInspectorWidget;
class PhysicsInspectorWidget;
class ThermalInspectorWidget;
class ForcesInspectorWidget;
class GlobalsInspectorWidget;

class InspectorWidget : public QWidget {
    Q_OBJECT
public:
    explicit InspectorWidget(SceneManager* sceneMgr, QWidget* parent = nullptr);

    void loadObject(SceneObject* obj);
    void unloadObject();

private slots:
    void refresh();

private:
    SceneManager* sceneManager;
    SceneObject* currentObject = nullptr;

    QVBoxLayout* mainLayout;
    QTabWidget* inspectorTabs;
    int objectTabIndex = -1;
    int thermalTabIndex = -1;
    int sceneTabIndex = -1;
    QTimer refreshTimer;

    TransformInspectorWidget* transformWidget;
    PhysicsInspectorWidget* physicsWidget;
    ThermalInspectorWidget* thermalWidget;
    ForcesInspectorWidget* forcesWidget;
    GlobalsInspectorWidget* globalsWidget;
};
