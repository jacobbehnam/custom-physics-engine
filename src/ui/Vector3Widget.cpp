#include "Vector3Widget.h"

Vector3Widget::Vector3Widget(const QString& suffix, QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);

    auto setupSpin = [&](QDoubleSpinBox*& spin) {
        spin = new QDoubleSpinBox();
        spin->setRange(-10000.0, 10000.0);
        spin->setDecimals(2);
        spin->setButtonSymbols(QAbstractSpinBox::NoButtons); // Clean look
        spin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        spin->setSuffix(suffix);
        layout->addWidget(spin);

        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            emit valueChanged(getValue());
        });
    };

    setupSpin(xSpin);
    setupSpin(ySpin);
    setupSpin(zSpin);
}

glm::vec3 Vector3Widget::getValue() const {
    return glm::vec3(xSpin->value(), ySpin->value(), zSpin->value());
}

void Vector3Widget::setValue(const glm::vec3& v) {
    QSignalBlocker b1(xSpin); QSignalBlocker b2(ySpin); QSignalBlocker b3(zSpin);
    xSpin->setValue(v.x);
    ySpin->setValue(v.y);
    zSpin->setValue(v.z);
}