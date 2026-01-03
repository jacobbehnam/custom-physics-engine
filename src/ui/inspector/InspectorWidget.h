#pragma once
#include <QWidget>
#include <QTimer>
#include <QVBoxLayout>

class SceneManager;
class SceneObject;

class TransformInspectorWidget;
class PhysicsInspectorWidget;
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
    QTimer refreshTimer;

    TransformInspectorWidget* transformWidget;
    PhysicsInspectorWidget* physicsWidget;
    ForcesInspectorWidget* forcesWidget;
    GlobalsInspectorWidget* globalsWidget;
};