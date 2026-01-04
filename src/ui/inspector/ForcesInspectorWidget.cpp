#include "ForcesInspectorWidget.h"
#include "graphics/core/SceneObject.h"
#include "physics/PhysicsBody.h"

ForcesInspectorWidget::ForcesInspectorWidget(QWidget* parent) : IInspectorSection(parent) {
    layout = new QFormLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);
    setLayout(layout);
}

Physics::PhysicsBody* ForcesInspectorWidget::getBody() const {
    if (!selectedObject) return nullptr;
    return selectedObject->getPhysicsBody();
}

void ForcesInspectorWidget::load(SceneObject* object) {
    selectedObject = object;
    clearRows();

    auto* body = getBody();
    if (!body) {
        setVisible(false);
        return;
    }

    setVisible(true);

    for (auto const& [name, val] : body->getAllForces(BodyLock::NOLOCK)) {
        std::string forceName = name;

        bool isReadOnly = (forceName == "Gravity" || forceName == "Normal");

        InspectorRow row(QString::fromStdString(forceName), this);

        std::function<void(glm::vec3)> setter = nullptr;
        if (!isReadOnly) {
            setter = [this, forceName](glm::vec3 v) {
                if (auto* b = getBody()) b->setForce(forceName, v, BodyLock::NOLOCK);
            };
        }

        row.addVec3(
            [this, forceName]() {
                auto* b = getBody();
                return b ? b->getForce(forceName, BodyLock::NOLOCK) : glm::vec3(0.0f);
            },
            setter,
            "N"
        );

        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }

    {
        InspectorRow net("Net Force", this);
        net.addVec3(
            [this]() {
                auto* b = getBody();
                if (!b) return glm::vec3(0.0f);

                glm::vec3 sum(0.0f);
                for (const auto& [_, f] : b->getAllForces(BodyLock::NOLOCK)) {
                    sum += f;
                }
                return sum;
            },
            nullptr,
            "N"
        );
        layout->addRow(net.getLabel(), net.getEditor());
        rows.push_back(std::move(net));
    }
}

void ForcesInspectorWidget::unload() {
    selectedObject = nullptr;
    clearRows();
    setVisible(false);
}

void ForcesInspectorWidget::refresh() {
    if (!getBody()) {
        setVisible(false);
        return;
    }

    for (auto& row : rows) {
        row.refresh();
    }
}

void ForcesInspectorWidget::clearRows() {
    while (layout->rowCount() > 0) {
        layout->removeRow(0);
    }
    rows.clear();
}