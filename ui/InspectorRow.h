#pragma once
#include <vector>
#include <functional>
#include <QWidget>
#include <QString>
#include <QHBoxLayout>
#include <glm/glm.hpp>

class InspectorRow {
public:
    explicit InspectorRow(const QString &lbl, QWidget* parent = nullptr);
    // Pass through constructor for complex widgets
    InspectorRow(const QString &lbl, QWidget* customEditor, std::function<void()> updateLogic);

    InspectorRow& addVec3(std::function<glm::vec3()> get, std::function<void(glm::vec3)> set);
    InspectorRow& addScalar(std::function<float()> get, std::function<void(float)> set);
    InspectorRow& addCheckbox(std::function<bool()> get, std::function<void(bool)> set);

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