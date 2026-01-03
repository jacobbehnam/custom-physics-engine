#include "TransformInspectorWidget.h"

#include <QFormLayout>
#include "graphics/core/SceneObject.h"

TransformInspectorWidget::TransformInspectorWidget(QWidget* parent) : IInspectorSection(parent) {
    layout = new QFormLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);
    setLayout(layout);
    createUiComponents();
}

void TransformInspectorWidget::load(SceneObject* object) {
    selectedObject = object;

    this->setVisible(true);
    this->setEnabled(true);

    refresh();
}

void TransformInspectorWidget::unload() {
    selectedObject = nullptr;
    this->setEnabled(false);
}

void TransformInspectorWidget::refresh() {
    for (auto& row : rows)
        row.refresh();
}

void TransformInspectorWidget::createUiComponents() {
    {
        InspectorRow row("Position", this);
        row.addVec3(
            [this]() {
                return selectedObject ? selectedObject->getPosition() : glm::vec3(0.0f);
            },
            [this](glm::vec3 v) {
                if (selectedObject) selectedObject->setPosition(v);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }

    {
        InspectorRow row("Rotation", this);
        row.addVec3(
            [this]() {
                return selectedObject ? selectedObject->getRotation() : glm::vec3(0.0f);
            },
            [this](glm::vec3 v) {
                if (selectedObject) selectedObject->setRotation(v);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }

    {
        InspectorRow row("Scale", this);
        row.addVec3(
            [this]() {
                return selectedObject ? selectedObject->getScale() : glm::vec3(1.0f);
            },
            [this](glm::vec3 v) {
                if (selectedObject) selectedObject->setScale(v);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
}