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
    QCheckBox* m_rayRequireGpuBox;
    QDoubleSpinBox* m_rayResScaleBox;
    QCheckBox* m_enableSunBox;
};