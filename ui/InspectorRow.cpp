#include "InspectorRow.h"
#include <QCheckBox>
#include "Vector3Widget.h"
#include "ScalarWidget.h"

InspectorRow::InspectorRow(const QString &lbl, QWidget* parent) : label(lbl) {
    container = new QWidget(parent);
    layout = new QHBoxLayout(container);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(5);
}

InspectorRow::InspectorRow(const QString &lbl, QWidget* customEditor, std::function<void()> updateLogic) : label(lbl), container(customEditor) {
    if (updateLogic) {
        updaters.push_back(updateLogic);
    }
}

InspectorRow& InspectorRow::addCheckbox(std::function<bool()> get, std::function<void(bool)> set) {
    QCheckBox* cb = new QCheckBox();
    layout->addWidget(cb);

    QObject::connect(cb, &QCheckBox::toggled, [set](bool checked){
        if (set) set(checked);
    });

    updaters.emplace_back([cb, get]() {
        if (!get) return;
        bool val = get();
        if (cb->isChecked() != val) {
            const QSignalBlocker blocker(cb);
            cb->setChecked(val);
        }
    });

    return *this;
}

InspectorRow& InspectorRow::addVec3(std::function<glm::vec3()> get, std::function<void(glm::vec3)> set) {
    Vector3Widget* vec = new Vector3Widget();
    layout->addWidget(vec);

    QObject::connect(vec, &Vector3Widget::valueChanged, [set](glm::vec3 v){
        if (set) set(v);
    });

    updaters.emplace_back([vec, get]() {
        if (!get) return;
        vec->setValue(get());
    });

    return *this;
}

InspectorRow &InspectorRow::addScalar(std::function<float()> get, std::function<void(float)> set) {
    ScalarWidget* scalar = new ScalarWidget();
    layout->addWidget(scalar);

    QObject::connect(scalar, &ScalarWidget::valueChanged, [set](float f){
        if (set) set(f);
    });

    updaters.emplace_back([scalar, get]() {
        if (!get) return;
        scalar->setValue(get());
    });

    return *this;
}

