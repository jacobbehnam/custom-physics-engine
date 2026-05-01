#include "ThermalInspectorWidget.h"
#include "graphics/core/SceneObject.h"
#include "physics/PhysicsBody.h"
#include "physics/ThermalProperties.h"

ThermalInspectorWidget::ThermalInspectorWidget(QWidget* parent) : IInspectorSection(parent) {
    layout = new QFormLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);
    setLayout(layout);

    createUiComponents();
}

Physics::PhysicsBody* ThermalInspectorWidget::getBody() const {
    if (!selectedObject) return nullptr;
    return selectedObject->getPhysicsBody();
}

void ThermalInspectorWidget::createUiComponents() {
    {
        InspectorRow row("Temperature", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? static_cast<float>(b->getThermalProperties(BodyLock::NOLOCK).tempK) : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.tempK = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "K"
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Specific Heat", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).specificHeat : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.specificHeat = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "J/(kgK)"
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Emissivity", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).emissivity : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.emissivity = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            ""
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Convection", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).heatTransferCoeff : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.heatTransferCoeff = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "W/(m2K)"
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Conductivity", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).conductivity : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.conductivity = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "W/(mK)"
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
}

void ThermalInspectorWidget::load(SceneObject* object) {
    selectedObject = object;

    if (getBody()) {
        this->setVisible(true);
        this->setEnabled(true);
        refresh();
    } else {
        this->setVisible(false);
    }
}

void ThermalInspectorWidget::unload() {
    selectedObject = nullptr;
    this->setVisible(false);
}

void ThermalInspectorWidget::refresh() {
    if (!getBody()) {
        this->setVisible(false);
        return;
    }

    for (auto& row : rows) {
        row.refresh();
    }
}
