#include "InspectorWidget.h"
#include "graphics/core/SceneObject.h"

#include <glm/glm.hpp>
#include <qgroupbox.h>
#include <QLineEdit>
#include <qpushbutton.h>

#include "InspectorRow.h"

InspectorWidget::InspectorWidget(QWidget* parent) : QWidget(parent) {
    mainLayout = new QVBoxLayout(this);

    setMinimumWidth(350);

    refreshTimer.setInterval(100);
    connect(&refreshTimer, &QTimer::timeout, this, &InspectorWidget::refresh);
    refreshTimer.start();
}

void InspectorWidget::loadObject(SceneObject* obj) {
    unloadObject();
    currentObject = obj;

    QGroupBox* transformGroup = new QGroupBox("Transform");
    auto* layout = new QFormLayout(transformGroup);
    mainLayout->addWidget(transformGroup);
    rows.emplace_back("Position",
        [obj]()->glm::vec3{ return obj->getPosition(); },
        [obj](glm::vec3 v){ obj->setPosition(v); },
        this);

    if (IPhysicsBody* body = obj->getPhysicsBody()) {
        rows.emplace_back("Velocity",
            [body]()->glm::vec3{ return body->getVelocity(); },
            [body](glm::vec3 v){ body->setVelocity(v); },
            this);
    }
    QGroupBox* forcesGroup = new QGroupBox("Forces");
    auto* layout2 = new QFormLayout(forcesGroup);
    mainLayout->addWidget(forcesGroup);

    auto* addForceWidget = new QWidget(forcesGroup);
    auto* addForceLayout = new QHBoxLayout(addForceWidget);
    auto* addForceField = new QLineEdit("Add a force", addForceWidget);
    auto* addForceButton = new QPushButton(addForceWidget);
    addForceLayout->addWidget(addForceField);
    addForceLayout->addWidget(addForceButton);
    layout2->addWidget(addForceWidget);


    for (InspectorRow row : rows) {
        layout->addRow(row.getLabel(), row.getEditor());
    }
}

void InspectorWidget::unloadObject() {
    clearLayout();
    rows.clear();
}

void InspectorWidget::clearLayout() {
    QLayoutItem* item;
    while ((item = mainLayout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

void InspectorWidget::refresh() {
    for (InspectorRow row : rows) {
        row.update();
    }
}

