#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <ui/InspectorRow.h>

class SceneObject;

struct PropertyRow {
    QString label;
    QWidget* widget;
};

class InspectorWidget : public QWidget {
    Q_OBJECT
public:
    explicit InspectorWidget(QWidget* parent = nullptr);

    void loadObject(SceneObject* obj);
    void unloadObject();

private:
    SceneObject* currentObject = nullptr;
    QFormLayout* layout = nullptr;

    std::vector<InspectorRow> rows;

    void clearLayout(QFormLayout* layout);
};