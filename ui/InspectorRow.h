#pragma once
#include <qstring.h>
#include <qwidget.h>
#include <glm/glm.hpp>
#include <iostream>

class InspectorRow {
public:
    template<typename Getter, typename Setter, typename = std::enable_if_t<std::is_invocable_v<Getter>>>
    InspectorRow(const QString &lbl, Getter get, Setter set, QWidget *parent = nullptr) {
        label = lbl + ": ";
        using ValueT = std::decay_t<decltype(get())>;

        if constexpr (std::is_same_v<ValueT, bool>) {
            std::cout << "bool" << std::endl;
        }
        else if constexpr (std::is_arithmetic_v<ValueT>) {
            editor = makeScalarWidget(get, parent, set);
        }
        else if constexpr (std::is_same_v<ValueT, glm::vec3>) {
            editor = makeVec3Widget(get, parent, set);
        }
        else {
            static_assert(!sizeof(ValueT), "Unsupported type");
        }
    }

    template<typename Getter, typename = std::enable_if_t<std::is_invocable_v<Getter>>>
    InspectorRow(const QString &lbl, Getter get, QWidget *parent = nullptr) {
        label = lbl + ": ";
        using ValueT = std::decay_t<decltype(get())>;

        if constexpr (std::is_same_v<ValueT, bool>) {
            std::cout << "bool" << std::endl;
        }
        else if constexpr (std::is_arithmetic_v<ValueT>) {
            editor = makeScalarWidget(get, parent);
        }
        else if constexpr (std::is_same_v<ValueT, glm::vec3>) {
            editor = makeVec3Widget(get, parent);
        }
        else {
            static_assert(!sizeof(ValueT), "Unsupported type");
        }
    }

    // Pass through for custom widgets
    InspectorRow(const QString &lbl, QWidget* customEditor, std::function<void()> updateLogic = nullptr);

    QString getLabel() { return label; }
    QWidget* getEditor() { return editor; }
    void update() { pullFromObject(); }

private:
    QString label;
    QWidget* editor = nullptr;
    std::function<void()> pullFromObject;

    QWidget* makeVec3Widget(const std::function<glm::vec3()>& get, QWidget* parent = nullptr, std::function<void(glm::vec3)> set = nullptr);
    QWidget* makeScalarWidget(const std::function<float()>& get, QWidget* parent = nullptr, std::function<void(float)> set = nullptr);
};