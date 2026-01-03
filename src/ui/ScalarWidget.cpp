#include "ScalarWidget.h"

ScalarWidget::ScalarWidget(const QString &suffix, QWidget *parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    spin = new QDoubleSpinBox(this);
    spin->setRange(-10000.0, 10000.0);
    spin->setDecimals(2);
    spin->setSuffix(suffix);
    spin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    layout->addWidget(spin);

    connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ScalarWidget::valueChanged);
}

double ScalarWidget::getValue() const {
    return spin->value();return spin->value();
}

void ScalarWidget::setValue(double val) {
    if (spin->hasFocus())
        return;
    QSignalBlocker blocker(spin);
    spin->setValue(val);
}

void ScalarWidget::setRange(double min, double max) {
    spin->setRange(min, max);
}
