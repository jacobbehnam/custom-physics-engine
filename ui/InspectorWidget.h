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
    void unloadObject();

private:
    SceneObject* currentObject = nullptr;
    QFormLayout* layout = nullptr;

    QDoubleSpinBox* posX;
    QDoubleSpinBox* posY;
    QDoubleSpinBox* posZ;

    QDoubleSpinBox* velX;
    QDoubleSpinBox* velY;
    QDoubleSpinBox* velZ;
};