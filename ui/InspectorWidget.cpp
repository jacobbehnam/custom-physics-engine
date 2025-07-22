#include "InspectorWidget.h"
#include "graphics/core/SceneObject.h"

#include <glm/glm.hpp>

InspectorWidget::InspectorWidget(QWidget* parent) : QWidget(parent) {
    layout = new QFormLayout(this);

    // Horizontal layout for X, Y, Z
    QWidget* positionWidget = new QWidget(this);
    QHBoxLayout* posLayout = new QHBoxLayout(positionWidget);
    posLayout->setContentsMargins(0, 0, 0, 0);

    QWidget* velocityWidget = new QWidget(this);
    QHBoxLayout* velocityLayout = new QHBoxLayout(velocityWidget);
    velocityLayout->setContentsMargins(0, 0, 0, 0);

    posX = new QDoubleSpinBox;
    posY = new QDoubleSpinBox;
    posZ = new QDoubleSpinBox;

    velX = new QDoubleSpinBox;
    velY = new QDoubleSpinBox;
    velZ = new QDoubleSpinBox;

    for (QDoubleSpinBox* spin : {posX, posY, posZ}) {
        spin->setRange(-10000.0, 10000.0);
        spin->setDecimals(3);
        posLayout->addWidget(spin);
    }

    for (QDoubleSpinBox* spin : {velX, velY, velZ}) {
        spin->setRange(-10000.0, 10000.0);
        spin->setDecimals(3);
        velocityLayout->addWidget(spin);
    }

    layout->addRow("Position:", positionWidget);
    layout->setRowVisible(0, false);
    layout->addRow("Initial Velocity:", velocityWidget);
    layout->setRowVisible(1, false);

    // Connect all position spin boxes to update the object
    auto onPositionChanged = [=]() {
        if (!currentObject) return;
        glm::vec3 newPos(posX->value(), posY->value(), posZ->value());
        currentObject->setPosition(newPos);
    };

    auto onVelocityChanged = [=]() {
        if (!currentObject) return;
        glm::vec3 newVel(velX->value(), velY->value(), velZ->value());
        currentObject->physicsBody->setVelocity(newVel);
    };

    connect(posX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onPositionChanged);
    connect(posY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onPositionChanged);
    connect(posZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onPositionChanged);

    connect(velX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onVelocityChanged);
    connect(velY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onVelocityChanged);
    connect(velZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onVelocityChanged);
}

void InspectorWidget::loadObject(SceneObject* object) {
    layout->setRowVisible(0, true);
    layout->setRowVisible(1, true);
    currentObject = object;

    glm::vec3 pos = object->getPosition();
    glm::vec3 vel = object->physicsBody->getVelocity();

    posX->blockSignals(true);
    posY->blockSignals(true);
    posZ->blockSignals(true);
    velX->blockSignals(true);
    velY->blockSignals(true);
    velZ->blockSignals(true);

    posX->setValue(pos.x);
    posY->setValue(pos.y);
    posZ->setValue(pos.z);

    velX->setValue(vel.x);
    velY->setValue(vel.y);
    velZ->setValue(vel.z);

    velX->blockSignals(false);
    velY->blockSignals(false);
    velZ->blockSignals(false);
}

void InspectorWidget::unloadObject() {
    layout->setRowVisible(0, false);
}
