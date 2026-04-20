#include "CameraTab.h"
#include <QFormLayout>
#include <QDoubleSpinBox>
#include "ui/AppSettings.h"
#include "ui/settings/CameraSettingsGroup.h"

CameraTab::CameraTab(QWidget* parent) : QWidget(parent) {
    auto* layout = new QFormLayout(this);

    m_sensBox = new QDoubleSpinBox();
    m_sensBox->setRange(0.01, 2.0);
    m_sensBox->setSingleStep(0.01);
    
    m_speedBox = new QDoubleSpinBox();
    m_speedBox->setRange(0.1, 100.0);
    m_speedBox->setSingleStep(0.5);
    
    m_fovBox = new QDoubleSpinBox();
    m_fovBox->setRange(10.0, 120.0);
    m_fovBox->setSingleStep(1.0);

    auto& camGroup = AppSettings::getInstance().getGroup<CameraSettingsGroup>();
    m_sensBox->setValue(camGroup.mouseSensitivity);
    m_speedBox->setValue(camGroup.movementSpeed);
    m_fovBox->setValue(camGroup.fov);

    layout->addRow("Mouse Sensitivity:", m_sensBox);
    layout->addRow("Movement Speed:", m_speedBox);
    layout->addRow("Field of View (FOV):", m_fovBox);
}

void CameraTab::saveSettings() {
    auto& camGroupSave = AppSettings::getInstance().getGroup<CameraSettingsGroup>();
    camGroupSave.movementSpeed = static_cast<float>(m_speedBox->value());
    camGroupSave.mouseSensitivity = static_cast<float>(m_sensBox->value());
    camGroupSave.fov = static_cast<float>(m_fovBox->value());
}