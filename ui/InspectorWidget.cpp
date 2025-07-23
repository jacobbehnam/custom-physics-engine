#include "InspectorWidget.h"
#include "graphics/core/SceneObject.h"

#include <glm/glm.hpp>

#include "InspectorRow.h"

InspectorWidget::InspectorWidget(QWidget* parent) : QWidget(parent) {
    layout = new QFormLayout(this);
}

void InspectorWidget::loadObject(SceneObject* obj) {
    clearLayout(layout);
    rows.clear();
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
    // TODO
}

void InspectorWidget::clearLayout(QFormLayout* layout) {
    while (layout->rowCount() > 0) {
        layout->removeRow(0);
    }
}
