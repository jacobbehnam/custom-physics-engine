#include "ScalarWidget.h"

ScalarWidget::ScalarWidget(const QString &suffix, QWidget *parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_spin = new QDoubleSpinBox(this);
    m_spin->setRange(-10000.0, 10000.0);
    m_spin->setDecimals(2);
    m_spin->setSuffix(suffix);
    m_spin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    layout->addWidget(m_spin);

    connect(m_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ScalarWidget::valueChanged);
}

double ScalarWidget::getValue() const {
    return m_spin->value();return m_spin->value();
}

void ScalarWidget::setValue(double val) {
    QSignalBlocker blocker(m_spin);
    m_spin->setValue(val);
}

void ScalarWidget::setRange(double min, double max) {
    m_spin->setRange(min, max);
}
