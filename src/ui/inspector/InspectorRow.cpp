#include "InspectorRow.h"
#include <QCheckBox>
#include "../Vector3Widget.h"
#include "../ScalarWidget.h"

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

InspectorRow& InspectorRow::addVec3(const std::function<glm::vec3()> &get, const std::function<void(glm::vec3)> &set, const QString& unit, const std::function<void(Vector3Widget*)> &onInit) {
    auto* vec = new Vector3Widget(" " + unit);
    vec->setValue(get());
    layout->addWidget(vec);

    if (onInit) {
        onInit(vec);
    }

    QObject::connect(vec, &Vector3Widget::valueChanged, [set](glm::vec3 v){
        if (set) set(v);
    });

    updaters.emplace_back([vec, get]() {
        if (!get) return;
        vec->setValue(get());
    });

    return *this;
}

InspectorRow &InspectorRow::addScalar(const std::function<float()> &get, const std::function<void(float)> &set, const QString& unit, const std::function<void(ScalarWidget*)> &onInit) {
    auto* scalar = new ScalarWidget(" " + unit);
    scalar->setValue(get());
    layout->addWidget(scalar);

    if (onInit) {
        onInit(scalar);
    }

    QObject::connect(scalar, &ScalarWidget::valueChanged, [set](float f){
        if (set) set(f);
    });

    updaters.emplace_back([scalar, get]() {
        if (!get) return;
        scalar->setValue(get());
    });

    return *this;
}


InspectorRow& InspectorRow::addCheckbox(const std::function<bool()> &get, const std::function<void(bool)> &set, const std::function<void(QCheckBox*)> &onInit) {
    QCheckBox* cb = new QCheckBox();
    cb->setChecked(get());
    layout->addWidget(cb);

    if (onInit) {
        onInit(cb);
    }

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