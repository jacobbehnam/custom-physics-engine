#include "PhysicsInspectorWidget.h"
#include <QCheckBox>
#include "graphics/core/SceneObject.h"
#include "physics/PhysicsBody.h"
#include "ui/Vector3Widget.h"

PhysicsInspectorWidget::PhysicsInspectorWidget(QWidget* parent) : IInspectorSection(parent) {
    layout = new QFormLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);
    setLayout(layout);

    createUiComponents();
}

Physics::PhysicsBody* PhysicsInspectorWidget::getBody() const {
    if (!selectedObject) return nullptr;
    return selectedObject->getPhysicsBody();
}

void PhysicsInspectorWidget::createUiComponents() {
    {
        QCheckBox* unknownBox = nullptr;
        Vector3Widget* velWidget = nullptr;

        InspectorRow row("Velocity", this);

        row.addCheckbox(
            [this]() {
                auto* b = getBody();
                return b ? b->isUnknown("v0", BodyLock::NOLOCK) : false;
            },
            [this](bool val) {
                if (auto* b = getBody()) b->setUnknown("v0", val, BodyLock::NOLOCK);
            },
            [&](QCheckBox* cb) { unknownBox = cb; }
        );

        row.addVec3(
            [this]() {
                auto* b = getBody();
                return b ? b->getVelocity(BodyLock::NOLOCK) : glm::vec3(0.0f);
            },
            [this](glm::vec3 v) {
                if (auto* b = getBody()) b->setVelocity(v, BodyLock::NOLOCK);
            },
            [&](Vector3Widget* v) { velWidget = v; }
        );

        if (unknownBox && velWidget) {
            connect(unknownBox, &QCheckBox::toggled, velWidget, &QWidget::setDisabled);
            velWidget->setEnabled(!unknownBox->isChecked());
        }

        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }

    {
        InspectorRow row("Mass", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getMass(BodyLock::NOLOCK) : 0.0f;
            },
            [this](float m) {
                if (auto* b = getBody()) b->setMass(m, BodyLock::NOLOCK);
            }
        );

        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
}

void PhysicsInspectorWidget::load(SceneObject* object) {
    selectedObject = object;

    if (getBody()) {
        this->setVisible(true);
        this->setEnabled(true);
        refresh();
    } else {
        this->setVisible(false);
    }
}

void PhysicsInspectorWidget::unload() {
    selectedObject = nullptr;
    this->setVisible(false);
}

void PhysicsInspectorWidget::refresh() {
    if (!getBody()) {
        this->setVisible(false);
        return;
    }

    for (auto& row : rows) {
        row.refresh();
    }
}