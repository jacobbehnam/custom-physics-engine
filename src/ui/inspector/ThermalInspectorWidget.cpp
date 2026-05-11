#include "ThermalInspectorWidget.h"

#include <QFormLayout>
#include <QTabWidget>
#include <QVBoxLayout>

#include "graphics/core/SceneObject.h"
#include "physics/PhysicsBody.h"
#include "physics/ThermalProperties.h"
#include "ui/ScalarWidget.h"

ThermalInspectorWidget::ThermalInspectorWidget(QWidget* parent) : IInspectorSection(parent) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    tabs = new QTabWidget(this);
    rootLayout->addWidget(tabs);

    setLayout(rootLayout);
    createUiComponents();
}

Physics::PhysicsBody* ThermalInspectorWidget::getBody() const {
    if (!selectedObject) return nullptr;
    return selectedObject->getPhysicsBody();
}

QFormLayout* ThermalInspectorWidget::createTab(const QString& title) {
    auto* page = new QWidget(tabs);
    auto* layout = new QFormLayout(page);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);
    tabs->addTab(page, title);
    return layout;
}

void ThermalInspectorWidget::addThermalScalar(QFormLayout* target,
                                              const QString& label,
                                              const ThermalGetter& get,
                                              const ThermalSetter& set,
                                              const QString& unit,
                                              const ScalarInitializer& onInit) {
    InspectorRow row(label, this);
    row.addScalar(
        [this, get]() {
            auto* body = getBody();
            return body ? get(body->getThermalProperties(BodyLock::NOLOCK)) : 0.0;
        },
        [this, set](double value) {
            if (auto* body = getBody()) {
                auto props = body->getThermalProperties(BodyLock::NOLOCK);
                set(props, value);
                body->setThermalProperty(props, BodyLock::NOLOCK);
            }
        },
        unit,
        onInit
    );
    target->addRow(row.getLabel(), row.getEditor());
    rows.push_back(std::move(row));
}

void ThermalInspectorWidget::createUiComponents() {
    auto* state = createTab("State");
    addThermalScalar(state, "Temperature",
        [](const ThermalProperties& props) { return props.tempK; },
        [](ThermalProperties& props, double value) { props.tempK = value; },
        "K");
    addThermalScalar(state, "Heat Generation",
        [](const ThermalProperties& props) { return props.internalHeatPower; },
        [](ThermalProperties& props, double value) { props.internalHeatPower = value; },
        "W",
        [](ScalarWidget* widget) {
            widget->setRange(-1.0e30, 1.0e30);
            widget->setDecimals(3);
        });
    addThermalScalar(state, "External Flux",
        [](const ThermalProperties& props) { return props.externalHeatFlux; },
        [](ThermalProperties& props, double value) { props.externalHeatFlux = value; },
        "W/m2",
        [](ScalarWidget* widget) {
            widget->setRange(-1.0e18, 1.0e18);
            widget->setDecimals(3);
        });
    addThermalScalar(state, "Entropy",
        [](const ThermalProperties& props) { return props.entropyJPerK; },
        [](ThermalProperties& props, double value) { props.entropyJPerK = value; },
        "J/K",
        [](ScalarWidget* widget) {
            widget->setRange(-1.0e30, 1.0e30);
            widget->setDecimals(3);
        });
    addThermalScalar(state, "Reference Temp",
        [](const ThermalProperties& props) { return props.referenceTempK; },
        [](ThermalProperties& props, double value) { props.referenceTempK = static_cast<float>(value); },
        "K");

    auto* material = createTab("Material");
    addThermalScalar(material, "Specific Heat",
        [](const ThermalProperties& props) { return props.specificHeat; },
        [](ThermalProperties& props, double value) { props.specificHeat = static_cast<float>(value); },
        "J/(kgK)");
    addThermalScalar(material, "Specific Heat Coeff",
        [](const ThermalProperties& props) { return props.specificHeatTempCoeff; },
        [](ThermalProperties& props, double value) { props.specificHeatTempCoeff = static_cast<float>(value); },
        "1/K",
        [](ScalarWidget* widget) {
            widget->setRange(-1.0e3, 1.0e3);
            widget->setDecimals(8);
        });
    addThermalScalar(material, "Thermal Mass",
        [](const ThermalProperties& props) { return props.thermalMassFraction; },
        [](ThermalProperties& props, double value) { props.thermalMassFraction = static_cast<float>(value); });
    addThermalScalar(material, "Density",
        [](const ThermalProperties& props) { return props.density; },
        [](ThermalProperties& props, double value) { props.density = static_cast<float>(value); },
        "kg/m3",
        [](ScalarWidget* widget) {
            widget->setRange(0.0, 1.0e12);
            widget->setDecimals(3);
        });
    addThermalScalar(material, "Expansion Coeff",
        [](const ThermalProperties& props) { return props.linearExpansionCoeff; },
        [](ThermalProperties& props, double value) { props.linearExpansionCoeff = static_cast<float>(value); },
        "1/K",
        [](ScalarWidget* widget) {
            widget->setRange(-1.0e3, 1.0e3);
            widget->setDecimals(8);
        });

    auto* radiation = createTab("Radiation");
    addThermalScalar(radiation, "Emissivity",
        [](const ThermalProperties& props) { return props.emissivity; },
        [](ThermalProperties& props, double value) { props.emissivity = static_cast<float>(value); });
    addThermalScalar(radiation, "Emissivity Coeff",
        [](const ThermalProperties& props) { return props.emissivityTempCoeff; },
        [](ThermalProperties& props, double value) { props.emissivityTempCoeff = static_cast<float>(value); },
        "1/K",
        [](ScalarWidget* widget) {
            widget->setRange(-1.0e3, 1.0e3);
            widget->setDecimals(8);
        });
    addThermalScalar(radiation, "Visible Light",
        [](const ThermalProperties& props) { return props.visibleLightPower; },
        [](ThermalProperties& props, double value) { props.visibleLightPower = static_cast<float>(value); },
        "",
        [](ScalarWidget* widget) {
            widget->setRange(0.0, 1000.0);
            widget->setDecimals(3);
        });
    addThermalScalar(radiation, "Light Red",
        [](const ThermalProperties& props) { return props.visibleLightColor.r; },
        [](ThermalProperties& props, double value) { props.visibleLightColor.r = static_cast<float>(value); },
        "",
        [](ScalarWidget* widget) {
            widget->setRange(0.0, 1.0);
            widget->setDecimals(3);
        });
    addThermalScalar(radiation, "Light Green",
        [](const ThermalProperties& props) { return props.visibleLightColor.g; },
        [](ThermalProperties& props, double value) { props.visibleLightColor.g = static_cast<float>(value); },
        "",
        [](ScalarWidget* widget) {
            widget->setRange(0.0, 1.0);
            widget->setDecimals(3);
        });
    addThermalScalar(radiation, "Light Blue",
        [](const ThermalProperties& props) { return props.visibleLightColor.b; },
        [](ThermalProperties& props, double value) { props.visibleLightColor.b = static_cast<float>(value); },
        "",
        [](ScalarWidget* widget) {
            widget->setRange(0.0, 1.0);
            widget->setDecimals(3);
        });
    addThermalScalar(radiation, "Absorptivity",
        [](const ThermalProperties& props) { return props.absorptivity; },
        [](ThermalProperties& props, double value) { props.absorptivity = static_cast<float>(value); });
    addThermalScalar(radiation, "Absorptivity Coeff",
        [](const ThermalProperties& props) { return props.absorptivityTempCoeff; },
        [](ThermalProperties& props, double value) { props.absorptivityTempCoeff = static_cast<float>(value); },
        "1/K",
        [](ScalarWidget* widget) {
            widget->setRange(-1.0e3, 1.0e3);
            widget->setDecimals(8);
        });

    auto* transfer = createTab("Transfer");
    addThermalScalar(transfer, "Convection",
        [](const ThermalProperties& props) { return props.heatTransferCoeff; },
        [](ThermalProperties& props, double value) { props.heatTransferCoeff = static_cast<float>(value); },
        "W/(m2K)");
    addThermalScalar(transfer, "Conductivity",
        [](const ThermalProperties& props) { return props.conductivity; },
        [](ThermalProperties& props, double value) { props.conductivity = static_cast<float>(value); },
        "W/(mK)");
    addThermalScalar(transfer, "Conductivity Coeff",
        [](const ThermalProperties& props) { return props.conductivityTempCoeff; },
        [](ThermalProperties& props, double value) { props.conductivityTempCoeff = static_cast<float>(value); },
        "1/K",
        [](ScalarWidget* widget) {
            widget->setRange(-1.0e3, 1.0e3);
            widget->setDecimals(8);
        });

    auto* phase = createTab("Phase");
    addThermalScalar(phase, "Melting Point",
        [](const ThermalProperties& props) { return props.meltingPoint; },
        [](ThermalProperties& props, double value) { props.meltingPoint = static_cast<float>(value); },
        "K");
    addThermalScalar(phase, "Latent Fusion",
        [](const ThermalProperties& props) { return props.latentHeatFusion; },
        [](ThermalProperties& props, double value) { props.latentHeatFusion = static_cast<float>(value); },
        "J/kg",
        [](ScalarWidget* widget) {
            widget->setRange(0.0, 1.0e12);
            widget->setDecimals(3);
        });
    addThermalScalar(phase, "Fusion",
        [](const ThermalProperties& props) { return props.fusionProgress; },
        [](ThermalProperties& props, double value) { props.fusionProgress = static_cast<float>(value); },
        "",
        [](ScalarWidget* widget) {
            widget->setRange(0.0, 1.0);
            widget->setDecimals(4);
        });
    addThermalScalar(phase, "Boiling Point",
        [](const ThermalProperties& props) { return props.boilingPoint; },
        [](ThermalProperties& props, double value) { props.boilingPoint = static_cast<float>(value); },
        "K");
    addThermalScalar(phase, "Latent Vaporization",
        [](const ThermalProperties& props) { return props.latentHeatVaporization; },
        [](ThermalProperties& props, double value) { props.latentHeatVaporization = static_cast<float>(value); },
        "J/kg",
        [](ScalarWidget* widget) {
            widget->setRange(0.0, 1.0e12);
            widget->setDecimals(3);
        });
    addThermalScalar(phase, "Vaporization",
        [](const ThermalProperties& props) { return props.vaporizationProgress; },
        [](ThermalProperties& props, double value) { props.vaporizationProgress = static_cast<float>(value); },
        "",
        [](ScalarWidget* widget) {
            widget->setRange(0.0, 1.0);
            widget->setDecimals(4);
        });
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
