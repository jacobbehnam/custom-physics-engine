#include "DebugTab.h"
#include <QFormLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include "ui/AppSettings.h"
#include "ui/settings/DebugSettings.h"

DebugTab::DebugTab(QWidget* parent) : QWidget(parent) {
    auto* layout = new QFormLayout(this);

    m_showTrailsBox = new QCheckBox();
    m_trailTimeBox = new QDoubleSpinBox();
    m_trailTimeBox->setRange(0.1, 60.0);
    m_trailTimeBox->setSingleStep(0.5);

    m_showForcesBox = new QCheckBox();
    m_showCollidersBox = new QCheckBox();
    m_rayTracedBox = new QCheckBox();
    m_rayRequireGpuBox = new QCheckBox();
    m_rayResScaleBox = new QDoubleSpinBox();
    m_rayResScaleBox->setRange(0.25, 1.0);
    m_rayResScaleBox->setSingleStep(0.05);
    m_rayResScaleBox->setDecimals(2);
    m_rayResScaleBox->setToolTip(tr("Ray trace at this fraction of window size, then upscale. Lower is faster."));
    m_rayRequireGpuBox->setToolTip(tr("When enabled, the ray traced view is disabled unless the GPU compute backend is available."));

    auto& dbgGroup = AppSettings::getInstance().getGroup<DebugSettings>();
    m_showTrailsBox->setChecked(dbgGroup.showAllPathTrails);
    m_trailTimeBox->setValue(dbgGroup.pathTrailTime);
    m_showForcesBox->setChecked(dbgGroup.showForces);
    m_showCollidersBox->setChecked(dbgGroup.showColliders);
    m_rayTracedBox->setChecked(dbgGroup.useRayTraced);
    m_rayRequireGpuBox->setChecked(dbgGroup.rayTraceRequireGpu);
    m_rayResScaleBox->setValue(static_cast<double>(dbgGroup.rayTraceResolutionScale));

    layout->addRow("Show All Path Trails:", m_showTrailsBox);
    layout->addRow("Path Trail Time (seconds):", m_trailTimeBox);
    layout->addRow("Show Forces:", m_showForcesBox);
    layout->addRow("Show Colliders:", m_showCollidersBox);
    layout->addRow("GPU Ray Traced View:", m_rayTracedBox);
    layout->addRow("Require GPU backend:", m_rayRequireGpuBox);
    layout->addRow("Ray trace resolution scale:", m_rayResScaleBox);
}

void DebugTab::saveSettings() {
    auto& dbgGroupSave = AppSettings::getInstance().getGroup<DebugSettings>();
    dbgGroupSave.showAllPathTrails = m_showTrailsBox->isChecked();
    dbgGroupSave.pathTrailTime = m_trailTimeBox->value();
    dbgGroupSave.showForces = m_showForcesBox->isChecked();
    dbgGroupSave.showColliders = m_showCollidersBox->isChecked();
    dbgGroupSave.useRayTraced = m_rayTracedBox->isChecked();
    dbgGroupSave.rayTraceRequireGpu = m_rayRequireGpuBox->isChecked();
    dbgGroupSave.rayTraceResolutionScale = static_cast<float>(m_rayResScaleBox->value());
}
