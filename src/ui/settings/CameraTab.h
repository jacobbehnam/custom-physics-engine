#pragma once
#include <QWidget>
#include "ISettingsTab.h"

class QDoubleSpinBox;

class CameraTab : public QWidget, public ISettingsTab {
    Q_OBJECT
public:
    explicit CameraTab(QWidget* parent = nullptr);
    void saveSettings() override;

private:
    QDoubleSpinBox* m_sensBox;
    QDoubleSpinBox* m_speedBox;
    QDoubleSpinBox* m_fovBox;
};