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

    auto& dbgGroup = AppSettings::getInstance().getGroup<DebugSettings>();
    m_showTrailsBox->setChecked(dbgGroup.showAllPathTrails);
    m_trailTimeBox->setValue(dbgGroup.pathTrailTime);
    m_showForcesBox->setChecked(dbgGroup.showForces);
    m_showCollidersBox->setChecked(dbgGroup.showColliders);

    layout->addRow("Show All Path Trails:", m_showTrailsBox);
    layout->addRow("Path Trail Time (seconds):", m_trailTimeBox);
    layout->addRow("Show Forces:", m_showForcesBox);
    layout->addRow("Show Colliders:", m_showCollidersBox);
}

void DebugTab::saveSettings() {
    auto& dbgGroupSave = AppSettings::getInstance().getGroup<DebugSettings>();
    dbgGroupSave.showAllPathTrails = m_showTrailsBox->isChecked();
    dbgGroupSave.pathTrailTime = m_trailTimeBox->value();
    dbgGroupSave.showForces = m_showForcesBox->isChecked();
    dbgGroupSave.showColliders = m_showCollidersBox->isChecked();
}