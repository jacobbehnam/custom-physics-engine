#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QDialogButtonBox>

#include "ui/AppSettings.h"
#include "ui/settings/ISettingsTab.h"
#include "ui/settings/CameraTab.h"
#include "ui/settings/DebugTab.h"

SettingsDialog::SettingsDialog(QWidget* parent) 
    : QDialog(parent) {
    setWindowTitle("Settings");
    resize(300, 200);

    auto* mainLayout = new QVBoxLayout(this);
    auto* tabWidget = new QTabWidget(this);
    
    auto* cameraTab = new CameraTab(this);
    tabWidget->addTab(cameraTab, "Camera");
    m_tabs.push_back(cameraTab);
    
    auto* debugTab = new DebugTab(this);
    tabWidget->addTab(debugTab, "Debug");
    m_tabs.push_back(debugTab);

    mainLayout->addWidget(tabWidget);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, [this]() {
        for (auto* tab : m_tabs) {
            tab->saveSettings();
        }

        QSettings qSettings;
        AppSettings::getInstance().save(qSettings);
        emit settingsSaved();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
