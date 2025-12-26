#pragma once
#include <vector>
#include <functional>
#include <QWidget>
#include <QString>
#include <QHBoxLayout>
#include <glm/glm.hpp>

class QCheckBox;
class Vector3Widget;
class ScalarWidget;

class InspectorRow {
public:
    explicit InspectorRow(const QString &lbl, QWidget* parent = nullptr);
    // Pass through constructor for complex widgets
    InspectorRow(const QString &lbl, QWidget* customEditor, std::function<void()> updateLogic);

    InspectorRow& addVec3(const std::function<glm::vec3()> &get, const std::function<void(glm::vec3)> &set, const std::function<void(Vector3Widget*)> &onInit = nullptr);
    InspectorRow& addScalar(const std::function<float()> &get, const std::function<void(float)> &set, const std::function<void(ScalarWidget*)> &onInit = nullptr);
    InspectorRow& addCheckbox(const std::function<bool()> &get, const std::function<void(bool)> &set, const std::function<void(QCheckBox*)> &onInit = nullptr);

    QString getLabel() const { return label; }
    QWidget* getEditor() const { return container; }

    void update() {
        for (auto& fn : updaters) {
            fn();
        }
    }

private:
    QString label;
    QWidget* container;
    QHBoxLayout* layout;

    std::vector<std::function<void()>> updaters;
};
