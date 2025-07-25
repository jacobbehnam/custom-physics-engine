#include "InspectorRow.h"

#include <qboxlayout.h>
#include <QDoubleSpinBox>
#include <type_traits>

QWidget* InspectorRow::makeVec3Widget(std::function<glm::vec3()> get, QWidget *parent, std::function<void(glm::vec3)> set) {
    QWidget* container = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    QDoubleSpinBox* xSpin = new QDoubleSpinBox(container);
    QDoubleSpinBox* ySpin = new QDoubleSpinBox(container);
    QDoubleSpinBox* zSpin = new QDoubleSpinBox(container);

    for (auto* spin : { xSpin, ySpin, zSpin }) {
        spin->setRange(-10000.0, 10000.0);
        spin->setDecimals(2);
        spin->setSingleStep(0.1);
        spin->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        layout->addWidget(spin);
    }

    glm::vec3 value = get();
    xSpin->setValue(value.x);
    ySpin->setValue(value.y);
    zSpin->setValue(value.z);

    if (set != nullptr) {
        pushToObject = [=]() {
            glm::vec3 newVal{
                static_cast<float>(xSpin->value()),
                static_cast<float>(ySpin->value()),
                static_cast<float>(zSpin->value())
            };
            set(newVal);
        };

        auto connectSpin = [=](QDoubleSpinBox* spin) {
            QObject::connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), container, [=](double) {
                pushToObject();
            });
        };

        connectSpin(xSpin);
        connectSpin(ySpin);
        connectSpin(zSpin);
    } else {
        xSpin->setDisabled(true);
        ySpin->setDisabled(true);
        zSpin->setDisabled(true);
    }

    pullFromObject = [=]() {
        glm::vec3 updated = get();

        QSignalBlocker blockX(xSpin);
        QSignalBlocker blockY(ySpin);
        QSignalBlocker blockZ(zSpin);

        xSpin->setValue(updated.x);
        ySpin->setValue(updated.y);
        zSpin->setValue(updated.z);
    };

    return container;
}
