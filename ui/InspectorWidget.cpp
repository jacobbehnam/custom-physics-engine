#include "InspectorWidget.h"
#include "graphics/core/SceneObject.h"

#include <glm/glm.hpp>

InspectorWidget::InspectorWidget(QWidget* parent) : QWidget(parent) {
    layout = new QFormLayout(this);

    // Horizontal layout for X, Y, Z
    QWidget* positionWidget = new QWidget(this);
    QHBoxLayout* posLayout = new QHBoxLayout(positionWidget);
    posLayout->setContentsMargins(0, 0, 0, 0);

    posX = new QDoubleSpinBox;
    posY = new QDoubleSpinBox;
    posZ = new QDoubleSpinBox;

    for (QDoubleSpinBox* spin : {posX, posY, posZ}) {
        spin->setRange(-10000.0, 10000.0);
        spin->setDecimals(3);
        posLayout->addWidget(spin);
    }

    layout->addRow("Position:", positionWidget);
    layout->setRowVisible(0, false);

    // Connect all position spin boxes to update the object
    auto onPositionChanged = [=]() {
        if (!currentObject) return;
        glm::vec3 newPos(posX->value(), posY->value(), posZ->value());
        currentObject->setPosition(newPos);
    };

    connect(posX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onPositionChanged);
    connect(posY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onPositionChanged);
    connect(posZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onPositionChanged);
}

void InspectorWidget::loadObject(SceneObject* object) {
    layout->setRowVisible(0, true);
    currentObject = object;

    glm::vec3 pos = object->getPosition();

    posX->blockSignals(true);
    posY->blockSignals(true);
    posZ->blockSignals(true);

    posX->setValue(pos.x);
    posY->setValue(pos.y);
    posZ->setValue(pos.z);

    posX->blockSignals(false);
    posY->blockSignals(false);
    posZ->blockSignals(false);
}

void InspectorWidget::unloadObject() {
    layout->setRowVisible(0, false);
}
