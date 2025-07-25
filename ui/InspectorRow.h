#pragma once
#include <qstring.h>
#include <qwidget.h>
#include <glm/glm.hpp>
#include <iostream>

class InspectorRow {
public:
    template<typename Getter, typename Setter>
    InspectorRow(const QString &lbl, Getter get, Setter set, QWidget *parent = nullptr) {
        label = lbl + ": ";
        using ValueT = std::decay_t<decltype(get())>;

        if constexpr (std::is_same_v<ValueT, bool>) {
            std::cout << "bool" << std::endl;
        }
        else if constexpr (std::is_arithmetic_v<ValueT>) {
            std::cout << "scalar" << std::endl;
        }
        else if constexpr (std::is_same_v<ValueT, glm::vec3>) {
            editor = makeVec3Widget(get, parent, set);
        }
        else {
            static_assert(!sizeof(ValueT), "Unsupported type");
        }
    }

    template<typename Getter>
    InspectorRow(const QString &lbl, Getter get, QWidget *parent = nullptr) {
        label = lbl + ": ";
        using ValueT = std::decay_t<decltype(get())>;

        if constexpr (std::is_same_v<ValueT, bool>) {
            std::cout << "bool" << std::endl;
        }
        else if constexpr (std::is_arithmetic_v<ValueT>) {
            std::cout << "scalar" << std::endl;
        }
        else if constexpr (std::is_same_v<ValueT, glm::vec3>) {
            editor = makeVec3Widget(get, parent);
        }
        else {
            static_assert(!sizeof(ValueT), "Unsupported type");
        }
    }

    QString getLabel() { return label; }
    QWidget* getEditor() { return editor; }
    void update() { pullFromObject(); }

private:
    QString label;
    QWidget* editor = nullptr;
    std::function<void()> pullFromObject;

    QWidget* makeVec3Widget(std::function<glm::vec3()> get, QWidget* parent = nullptr, std::function<void(glm::vec3)> set = nullptr);
};