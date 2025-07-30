#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QTimer>
#include <ui/InspectorRow.h>

#include "graphics/core/SceneManager.h"
#include "physics/PhysicsSystem.h"

class IPhysicsBody;
class SceneObject;

struct PropertyRow {
    QString label;
    QWidget* widget;
};

class InspectorWidget : public QWidget {
    Q_OBJECT
public:
    explicit InspectorWidget(SceneManager* sceneManager, QWidget* parent = nullptr);

    void loadObject(SceneObject* obj);
    void unloadObject(bool loadGlobals = true);

private slots:
    void refresh();

private:
    SceneManager* sceneManager = nullptr;
    SceneObject* currentObject = nullptr;
    QVBoxLayout* mainLayout = nullptr;
    QTimer refreshTimer;

    // Storing in a vector keeps the InspectorRows alive for as long as InspectorWidget is alive
    std::vector<InspectorRow> globalsRows;
    std::vector<InspectorRow> transformRows;
    std::vector<InspectorRow> forceRows;

    void clearLayout(QVBoxLayout* layout);
    void clearLayout(QFormLayout* layout);
    void populateForces(IPhysicsBody* body, QFormLayout* layout);
    void populateGlobals(QVBoxLayout* layout);
};