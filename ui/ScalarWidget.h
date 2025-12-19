#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include <QDoubleSpinBox>

class ScalarWidget : public QWidget {
    Q_OBJECT

public:
    explicit ScalarWidget(const QString& suffix = "", QWidget* parent = nullptr);

    double getValue() const;
    void setValue(double val);

    void setRange(double min, double max);
signals:
    void valueChanged(double newValue);

private:
    QDoubleSpinBox* m_spin;
};
