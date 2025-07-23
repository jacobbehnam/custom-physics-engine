#include "InspectorWidget.h"
#include "graphics/core/SceneObject.h"

#include <glm/glm.hpp>

#include "InspectorRow.h"

InspectorWidget::InspectorWidget(QWidget* parent) : QWidget(parent) {
    layout = new QFormLayout(this);

    refreshTimer.setInterval(100);
    connect(&refreshTimer, &QTimer::timeout, this, &InspectorWidget::refresh);
    refreshTimer.start();
}

void InspectorWidget::loadObject(SceneObject* obj) {
    currentObject = obj;

    rows.emplace_back("Position",
        [obj]()->glm::vec3{ return obj->getPosition(); },
        [obj](glm::vec3 v){ obj->setPosition(v); },
        this);

    for (InspectorRow row : rows) {
        layout->addRow(row.getLabel(), row.getEditor());
    }
}

void InspectorWidget::unloadObject() {
    clearLayout();
    rows.clear();
}

void InspectorWidget::clearLayout() {
    while (layout->rowCount() > 0) {
        layout->removeRow(0);
    }
}

void InspectorWidget::refresh() {
    for (InspectorRow row : rows) {
        row.update();
    }
}

