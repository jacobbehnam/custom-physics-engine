#pragma once
#include <QWidget>
#include "ISettingsTab.h"

class QCheckBox;
class QDoubleSpinBox;

class DebugTab : public QWidget, public ISettingsTab {
    Q_OBJECT
public:
    explicit DebugTab(QWidget* parent = nullptr);
    void saveSettings() override;

private:
    QCheckBox* m_showTrailsBox;
    QDoubleSpinBox* m_trailTimeBox;
    QCheckBox* m_showForcesBox;
    QCheckBox* m_showCollidersBox;
};
