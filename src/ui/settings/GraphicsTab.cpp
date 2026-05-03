#include "GraphicsTab.h"
#include <QFormLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include "ui/AppSettings.h"
#include "ui/settings/GraphicsSettings.h"

GraphicsTab::GraphicsTab(QWidget* parent) : QWidget(parent) {
    auto* layout = new QFormLayout(this);

    m_rayTracedBox = new QCheckBox();
    m_rayResScaleBox = new QDoubleSpinBox();
    m_enableGlobalLightBox = new QCheckBox();
    m_rayResScaleBox->setRange(
        GraphicsSettings::kMinRayTraceResolutionScale,
        GraphicsSettings::kMaxRayTraceResolutionScale);
    m_rayResScaleBox->setSingleStep(0.05);
    m_rayResScaleBox->setDecimals(2);
    m_rayResScaleBox->setToolTip(tr("Ray trace at this fraction of window size, then upscale. Lower is faster."));

    auto& visGroup = AppSettings::getInstance().getGroup<GraphicsSettings>();
    m_rayTracedBox->setChecked(visGroup.useRayTraced);
    m_rayResScaleBox->setValue(static_cast<double>(visGroup.rayTraceResolutionScale));
    m_enableGlobalLightBox->setChecked(visGroup.enableGlobalLight);

    layout->addRow("GPU Ray Traced View:", m_rayTracedBox);
    layout->addRow("Ray trace resolution scale:", m_rayResScaleBox);
    layout->addRow("Enable Global Light:", m_enableGlobalLightBox);
}

void GraphicsTab::saveSettings() {
    auto& visGroupSave = AppSettings::getInstance().getGroup<GraphicsSettings>();
    visGroupSave.useRayTraced = m_rayTracedBox->isChecked();
    visGroupSave.rayTraceResolutionScale = static_cast<float>(m_rayResScaleBox->value());
    visGroupSave.enableGlobalLight = m_enableGlobalLightBox->isChecked();
}
