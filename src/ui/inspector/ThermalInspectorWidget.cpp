#include "ThermalInspectorWidget.h"
#include "graphics/core/SceneObject.h"
#include "physics/PhysicsBody.h"
#include "physics/ThermalProperties.h"
#include "ui/ScalarWidget.h"

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
        InspectorRow row("Heat Generation", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).internalHeatPower : 0.0;
            },
            [this](double val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.internalHeatPower = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "W",
            [](ScalarWidget* widget) {
                widget->setRange(-1.0e30, 1.0e30);
                widget->setDecimals(3);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("External Flux", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).externalHeatFlux : 0.0;
            },
            [this](double val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.externalHeatFlux = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "W/m2",
            [](ScalarWidget* widget) {
                widget->setRange(-1.0e18, 1.0e18);
                widget->setDecimals(3);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Entropy", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).entropyJPerK : 0.0;
            },
            [this](double val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.entropyJPerK = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "J/K",
            [](ScalarWidget* widget) {
                widget->setRange(-1.0e30, 1.0e30);
                widget->setDecimals(3);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Reference Temp", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).referenceTempK : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.referenceTempK = val;
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
        InspectorRow row("Specific Heat Coeff", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).specificHeatTempCoeff : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.specificHeatTempCoeff = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "1/K",
            [](ScalarWidget* widget) {
                widget->setRange(-1.0e3, 1.0e3);
                widget->setDecimals(8);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Thermal Mass", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).thermalMassFraction : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.thermalMassFraction = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            ""
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
        InspectorRow row("Emissivity Coeff", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).emissivityTempCoeff : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.emissivityTempCoeff = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "1/K",
            [](ScalarWidget* widget) {
                widget->setRange(-1.0e3, 1.0e3);
                widget->setDecimals(8);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Absorptivity", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).absorptivity : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.absorptivity = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            ""
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Absorptivity Coeff", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).absorptivityTempCoeff : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.absorptivityTempCoeff = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "1/K",
            [](ScalarWidget* widget) {
                widget->setRange(-1.0e3, 1.0e3);
                widget->setDecimals(8);
            }
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
    {
        InspectorRow row("Conductivity Coeff", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).conductivityTempCoeff : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.conductivityTempCoeff = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "1/K",
            [](ScalarWidget* widget) {
                widget->setRange(-1.0e3, 1.0e3);
                widget->setDecimals(8);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Density", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).density : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.density = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "kg/m3",
            [](ScalarWidget* widget) {
                widget->setRange(0.0, 1.0e12);
                widget->setDecimals(3);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Expansion Coeff", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).linearExpansionCoeff : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.linearExpansionCoeff = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "1/K",
            [](ScalarWidget* widget) {
                widget->setRange(-1.0e3, 1.0e3);
                widget->setDecimals(8);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Melting Point", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).meltingPoint : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.meltingPoint = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "K"
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Latent Fusion", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).latentHeatFusion : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.latentHeatFusion = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "J/kg",
            [](ScalarWidget* widget) {
                widget->setRange(0.0, 1.0e12);
                widget->setDecimals(3);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Fusion", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).fusionProgress : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.fusionProgress = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "",
            [](ScalarWidget* widget) {
                widget->setRange(0.0, 1.0);
                widget->setDecimals(4);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Boiling Point", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).boilingPoint : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.boilingPoint = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "K"
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Latent Vaporization", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).latentHeatVaporization : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.latentHeatVaporization = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "J/kg",
            [](ScalarWidget* widget) {
                widget->setRange(0.0, 1.0e12);
                widget->setDecimals(3);
            }
        );
        layout->addRow(row.getLabel(), row.getEditor());
        rows.push_back(std::move(row));
    }
    {
        InspectorRow row("Vaporization", this);
        row.addScalar(
            [this]() {
                auto* b = getBody();
                return b ? b->getThermalProperties(BodyLock::NOLOCK).vaporizationProgress : 0.0f;
            },
            [this](float val) {
                if (auto* b = getBody()) {
                    auto props = b->getThermalProperties(BodyLock::NOLOCK);
                    props.vaporizationProgress = val;
                    b->setThermalProperty(props, BodyLock::NOLOCK);
                }
            },
            "",
            [](ScalarWidget* widget) {
                widget->setRange(0.0, 1.0);
                widget->setDecimals(4);
            }
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
