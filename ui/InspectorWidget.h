#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>

class SceneObject;

class InspectorWidget : public QWidget {
    Q_OBJECT
public:
    explicit InspectorWidget(QWidget* parent = nullptr);

    void loadObject(SceneObject* object);

private:
    SceneObject* currentObject = nullptr;

    QDoubleSpinBox* posX;
    QDoubleSpinBox* posY;
    QDoubleSpinBox* posZ;
};