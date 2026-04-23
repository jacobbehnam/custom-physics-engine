#pragma once
#include <QDialog>
#include <vector>

class ISettingsTab;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

signals:
    void settingsSaved();

private:
    std::vector<ISettingsTab*> m_tabs;
};
