#pragma once
#include <QWidget>
#include "ISettingsTab.h"

class QCheckBox;
class QDoubleSpinBox;

class GraphicsTab : public QWidget, public ISettingsTab {
    Q_OBJECT
public:
    explicit GraphicsTab(QWidget* parent = nullptr);
    void saveSettings() override;

private:
    QCheckBox* m_rayTracedBox;
    QDoubleSpinBox* m_rayResScaleBox;
    QDoubleSpinBox* m_rayExposureBox;
    QCheckBox* m_enableGlobalLightBox;
};
