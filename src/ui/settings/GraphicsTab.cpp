#include "GraphicsTab.h"
#include <QFormLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include "ui/AppSettings.h"
#include "ui/settings/GraphicsSettings.h"

GraphicsTab::GraphicsTab(QWidget* parent) : QWidget(parent) {
    auto* layout = new QFormLayout(this);

    m_rayTracedBox = new QCheckBox();
    m_rayRequireGpuBox = new QCheckBox();
    m_rayResScaleBox = new QDoubleSpinBox();
    m_enableSunBox = new QCheckBox();
    m_rayResScaleBox->setRange(0.25, 1.0);
    m_rayResScaleBox->setSingleStep(0.05);
    m_rayResScaleBox->setDecimals(2);
    m_rayResScaleBox->setToolTip(tr("Ray trace at this fraction of window size, then upscale. Lower is faster."));
    m_rayRequireGpuBox->setToolTip(tr("When enabled, the ray traced view is disabled unless the GPU compute backend is available."));

    auto& visGroup = AppSettings::getInstance().getGroup<GraphicsSettings>();
    m_rayTracedBox->setChecked(visGroup.useRayTraced);
    m_rayRequireGpuBox->setChecked(visGroup.rayTraceRequireGpu);
    m_rayResScaleBox->setValue(static_cast<double>(visGroup.rayTraceResolutionScale));
    m_enableSunBox->setChecked(visGroup.enableSun);

    layout->addRow("GPU Ray Traced View:", m_rayTracedBox);
    layout->addRow("Require GPU backend:", m_rayRequireGpuBox);
    layout->addRow("Ray trace resolution scale:", m_rayResScaleBox);
    layout->addRow("Enable Sun:", m_enableSunBox);
}

void GraphicsTab::saveSettings() {
    auto& visGroupSave = AppSettings::getInstance().getGroup<GraphicsSettings>();
    visGroupSave.useRayTraced = m_rayTracedBox->isChecked();
    visGroupSave.rayTraceRequireGpu = m_rayRequireGpuBox->isChecked();
    visGroupSave.rayTraceResolutionScale = static_cast<float>(m_rayResScaleBox->value());
    visGroupSave.enableSun = m_enableSunBox->isChecked();
}