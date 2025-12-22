#include "InspectorRow.h"

#include <qboxlayout.h>

#include "ScalarWidget.h"
#include "Vector3Widget.h"

QWidget* InspectorRow::makeVec3Widget(const std::function<glm::vec3()>& get, QWidget *parent, std::function<void(glm::vec3)> set) {
    Vector3Widget* widget = new Vector3Widget("", parent);

    widget->setValue(get());

    if (set) {
        QObject::connect(widget, &Vector3Widget::valueChanged, [=](const glm::vec3& val) {
            set(val);
        });
    } else {
        widget->setEnabled(false);
    }

    pullFromObject = [=]() {
        widget->setValue(get());
    };

    return widget;
}

QWidget *InspectorRow::makeScalarWidget(const std::function<float()>& get, QWidget *parent, std::function<void(float)> set) {
    ScalarWidget* widget = new ScalarWidget("", parent);
    widget->setValue(get());

    if (set) {
        QObject::connect(widget, &ScalarWidget::valueChanged, [=](double val) {
            set(static_cast<float>(val));
        });
    } else {
        widget->setEnabled(false);
    }

    pullFromObject = [=]() {
        widget->setValue(get());
    };

    return widget;
}

InspectorRow::InspectorRow(const QString &lbl, QWidget *customEditor, std::function<void()> updateLogic) {
    label = lbl;
    editor = customEditor;

    if (updateLogic) pullFromObject = updateLogic;
    else pullFromObject = [](){};
}

