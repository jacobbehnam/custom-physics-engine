#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QTimer>
#include <ui/InspectorRow.h>

class IPhysicsBody;
class SceneObject;

struct PropertyRow {
    QString label;
    QWidget* widget;
};

class InspectorWidget : public QWidget {
    Q_OBJECT
public:
    explicit InspectorWidget(QWidget* parent = nullptr);

    void loadObject(SceneObject* obj);
    void unloadObject();

private slots:
    void refresh();

private:
    SceneObject* currentObject = nullptr;
    QVBoxLayout* mainLayout = nullptr;
    QTimer refreshTimer;

    // Storing in a vector keeps the InspectorRows alive for as long as InspectorWidget is alive
    std::vector<InspectorRow> transformRows;
    std::vector<InspectorRow> forceRows;

    void clearLayout(QVBoxLayout* layout);
    void clearLayout(QFormLayout* layout);
    void populateForces(IPhysicsBody* body, QFormLayout* layout);
};