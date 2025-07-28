#include "InspectorWidget.h"
#include "graphics/core/SceneObject.h"

#include <glm/glm.hpp>
#include <qgroupbox.h>
#include <QLineEdit>
#include <qpushbutton.h>

#include "InspectorRow.h"

InspectorWidget::InspectorWidget(QWidget* parent) : QWidget(parent) {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 0, 15, 0);

    setMinimumWidth(350);

    refreshTimer.setInterval(100);
    connect(&refreshTimer, &QTimer::timeout, this, &InspectorWidget::refresh);
    refreshTimer.start();
}

void InspectorWidget::loadObject(SceneObject* obj) {
    unloadObject();
    currentObject = obj;

    // Transform
    QGroupBox* transformGroup = new QGroupBox("Transform");
    auto* layout = new QFormLayout(transformGroup);
    mainLayout->addWidget(transformGroup);
    transformRows.emplace_back("Position",
        [obj]()->glm::vec3{ return obj->getPosition(); },
        [obj](glm::vec3 v){ obj->setPosition(v); },
        this);
    transformRows.emplace_back("Scale",
        [obj]()->glm::vec3{ return obj->getScale(); },
        [obj](glm::vec3 v) { obj->setScale(v); },
        this);
    // TODO: this works good enough for 2D but for 3D the conversion between euler angles and quaternions is wonky
    transformRows.emplace_back("Rotation",
        [obj]()->glm::vec3{ return glm::degrees(obj->getRotation()); },
        [obj](glm::vec3 v) { obj->setRotation(glm::radians(v)); },
        this);

    if (IPhysicsBody* body = obj->getPhysicsBody()) {
        transformRows.emplace_back("Velocity",
            [body]()->glm::vec3{ return body->getVelocity(); },
            [body](glm::vec3 v){ body->setVelocity(v); },
            this);
    }
    for (InspectorRow row : transformRows) {
        layout->addRow(row.getLabel(), row.getEditor());
    }

    // Forces
    QGroupBox* forcesGroup = new QGroupBox("Forces");
    auto* layout2 = new QVBoxLayout(forcesGroup);
    mainLayout->addWidget(forcesGroup);

    auto* addForceWidget = new QWidget(forcesGroup);
    auto* addForceLayout = new QHBoxLayout(addForceWidget);
    auto* addForceField = new QLineEdit("Add a force", addForceWidget);
    auto* addForceButton = new QPushButton(addForceWidget);

    addForceLayout->addWidget(addForceField);
    addForceLayout->addWidget(addForceButton);
    layout2->addWidget(addForceWidget);

    auto* forcesWidget = new QWidget(forcesGroup);
    auto* forcesLayout = new QFormLayout(forcesWidget);

    if (IPhysicsBody* body = obj->getPhysicsBody()) {
        populateForces(body, forcesLayout);
        connect(addForceButton, &QPushButton::clicked, this, [=]() {
            std::string text = addForceField->text().toStdString();
            if (text.empty()) return;
            if (std::isalnum(text.front()))
                text[0] = static_cast<char>(std::toupper(text[0]));
            body->setForce(text, glm::vec3(0.0f));
            populateForces(body, forcesLayout);
        });
    }
    layout2->addWidget(forcesWidget);
}

void InspectorWidget::populateForces(IPhysicsBody* body, QFormLayout *layout) {
    clearLayout(layout);
    forceRows.clear();

    for (auto const& [name, vec] : body->getAllForces()) {
        if (name == "Gravity" || name == "Normal") {
            forceRows.emplace_back(
              QString::fromStdString(name),
              [body, name](){ return body->getForce(name); }
            );
        } else {
            forceRows.emplace_back(
              QString::fromStdString(name),
              [body, name](){ return body->getForce(name); },
              [body, name](glm::vec3 v){ body->setForce(name, v); },
              this
            );
        }
        auto& row = forceRows.back();
        layout->addRow(row.getLabel(), row.getEditor());
    }

    forceRows.emplace_back(
      "Net Force",
      [body](){
        glm::vec3 sum{0.0f};
        for (auto const& [n, f] : body->getAllForces()) sum += f;
        return sum;
      }
    );
    auto& row = forceRows.back();
    layout->addRow(row.getLabel(), row.getEditor());
}


void InspectorWidget::unloadObject() {
    clearLayout(mainLayout);
    transformRows.clear();
    forceRows.clear();
}

void InspectorWidget::clearLayout(QVBoxLayout* layout) {
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item; // We are responsible for managing memory when we take a QLayoutItem
    }
}

void InspectorWidget::clearLayout(QFormLayout* layout) {
    while (layout->rowCount() > 0) {
        QLayoutItem* labelItem = layout->itemAt(0, QFormLayout::LabelRole);
        QLayoutItem* fieldItem = layout->itemAt(0, QFormLayout::FieldRole);

        if (labelItem && labelItem->widget()) {
            labelItem->widget()->deleteLater();
        }
        if (fieldItem && fieldItem->widget()) {
            fieldItem->widget()->deleteLater();
        }

        layout->removeRow(0);
    }
}

void InspectorWidget::refresh() {
    for (InspectorRow &row : transformRows) {
        row.update();
    }
    for (InspectorRow &row : forceRows) {
        row.update();
    }
}

