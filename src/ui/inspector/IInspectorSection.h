#pragma once
#include <QWidget>

class SceneObject;

class IInspectorSection : public QWidget {
    Q_OBJECT
public:
    explicit IInspectorSection(QWidget* parent = nullptr) : QWidget(parent) {}
    virtual void load(SceneObject* object) = 0;
    virtual void unload() = 0;
    virtual void refresh() = 0;

    virtual ~IInspectorSection() = default;

    IInspectorSection() = default;
};