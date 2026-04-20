#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>

#include "ui/AppSettings.h"
#include "ui/settings/CameraSettingsGroup.h"
#include "ui/settings/DebugSettings.h"

SettingsDialog::SettingsDialog(QWidget* parent) 
    : QDialog(parent) {
    setWindowTitle("Settings");
    resize(300, 200);

    auto* mainLayout = new QVBoxLayout(this);
    auto* tabWidget = new QTabWidget(this);
    
    // Camera Tab
    auto* cameraTab = new QWidget();
    auto* cameraLayout = new QFormLayout(cameraTab);

    auto* sensBox = new QDoubleSpinBox();
    sensBox->setRange(0.01, 2.0);
    sensBox->setSingleStep(0.01);
    
    auto* speedBox = new QDoubleSpinBox();
    speedBox->setRange(0.1, 100.0);
    speedBox->setSingleStep(0.5);
    
    auto* fovBox = new QDoubleSpinBox();
    fovBox->setRange(10.0, 120.0);
    fovBox->setSingleStep(1.0);

    auto& camGroup = AppSettings::getInstance().getGroup<CameraSettingsGroup>();
    sensBox->setValue(camGroup.mouseSensitivity);
    speedBox->setValue(camGroup.movementSpeed);
    fovBox->setValue(camGroup.fov);

    cameraLayout->addRow("Mouse Sensitivity:", sensBox);
    cameraLayout->addRow("Movement Speed:", speedBox);
    cameraLayout->addRow("Field of View (FOV):", fovBox);
    tabWidget->addTab(cameraTab, "Camera");
    
    // Debug Tab
    auto* debugTab = new QWidget();
    auto* debugLayout = new QFormLayout(debugTab);

    auto* showTrailsBox = new QCheckBox();
    auto* trailTimeBox = new QDoubleSpinBox();
    trailTimeBox->setRange(0.1, 60.0);
    trailTimeBox->setSingleStep(0.5);

    auto& dbgGroup = AppSettings::getInstance().getGroup<DebugSettings>();
    showTrailsBox->setChecked(dbgGroup.showAllPathTrails);
    trailTimeBox->setValue(dbgGroup.pathTrailTime);

    debugLayout->addRow("Show All Path Trails:", showTrailsBox);
    debugLayout->addRow("Path Trail Time (seconds):", trailTimeBox);
    tabWidget->addTab(debugTab, "Debug");

    mainLayout->addWidget(tabWidget);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, [=, this]() {
        auto& camGroupSave = AppSettings::getInstance().getGroup<CameraSettingsGroup>();
        camGroupSave.movementSpeed = static_cast<float>(speedBox->value());
        camGroupSave.mouseSensitivity = static_cast<float>(sensBox->value());
        camGroupSave.fov = static_cast<float>(fovBox->value());
        
        auto& dbgGroupSave = AppSettings::getInstance().getGroup<DebugSettings>();
        dbgGroupSave.showAllPathTrails = showTrailsBox->isChecked();
        dbgGroupSave.pathTrailTime = trailTimeBox->value();

        QSettings qSettings;
        AppSettings::getInstance().save(qSettings);
        emit settingsSaved();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
