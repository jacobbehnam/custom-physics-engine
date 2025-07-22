// HierarchyWidget.h
#pragma once

#include <QTreeWidget>
#include "graphics/core/SceneObject.h"

class HierarchyWidget : public QWidget {
    Q_OBJECT

public:
    explicit HierarchyWidget(QWidget* parent = nullptr);
    void addObject(SceneObject* obj);
    void removeObject(SceneObject* obj);
    //SceneObject* getSelectedObject() const;

    signals:
        void selectionChanged(SceneObject* previous, SceneObject* current);

public slots:
    void onItemSelectionChanged();
    void selectObject(SceneObject* obj);

private:
    QTreeWidget* tree;
    QTreeWidgetItem* previousItem = nullptr;

    static SceneObject* getObjectFromItem(QTreeWidgetItem* item);
};
