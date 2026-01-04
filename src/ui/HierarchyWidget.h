#pragma once

#include <QTreeWidget>
#include <QToolBar>
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
    void createObjectRequested(const CreationOptions& options);
    void renameObjectRequested(SceneObject* obj, const QString& newName);

public slots:
    void onItemNameChanged(QTreeWidgetItem* item, int column);
    void setObjectName(SceneObject* obj, const QString& name);
    void onItemSelectionChanged();
    void selectObject(SceneObject* obj);
    void showContextMenu(const QPoint& pos);

private:
    QTreeWidget* tree;
    QToolBar* toolBar;
    QTreeWidgetItem* previousItem = nullptr;

    static SceneObject* getObjectFromItem(QTreeWidgetItem* item);
    QString typeFor(SceneObject* obj);
};
