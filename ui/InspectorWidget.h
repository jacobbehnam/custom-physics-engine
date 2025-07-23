#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QTimer>
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

private slots:
    void refresh();

private:
    SceneObject* currentObject = nullptr;
    QFormLayout* layout = nullptr;
    QTimer refreshTimer;

    std::vector<InspectorRow> rows;

    void clearLayout();
};