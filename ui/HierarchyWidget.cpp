#include "HierarchyWidget.h"

#include <QVBoxLayout>

HierarchyWidget::HierarchyWidget(QWidget* parent) : QWidget(parent) {
    tree = new QTreeWidget(this);
    tree->setHeaderLabels({ "Name", "Type" });

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(tree);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    connect(tree, &QTreeWidget::itemSelectionChanged, this, &HierarchyWidget::onItemSelectionChanged);
}

void HierarchyWidget::addObject(SceneObject* obj) {
    auto* item = new QTreeWidgetItem();
    // TODO: derive name/type from obj
    item->setText(0, QString::fromStdString("Cube"));
    item->setText(1, QString::fromStdString("SceneObject"));
    item->setData(0, Qt::UserRole, QVariant::fromValue<void*>(obj));
    tree->addTopLevelItem(item);
}

void HierarchyWidget::selectObject(SceneObject* obj) {
    if (!obj) {
        tree->setCurrentItem(nullptr);
        tree->clearSelection();
        return;
    }

    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = tree->topLevelItem(i);
        if (getObjectFromItem(item) == obj) {
            tree->setCurrentItem(item); // will then call onItemSelectionChanged()
            break;
        }
    }
}

void HierarchyWidget::onItemSelectionChanged() {
    SceneObject* previous = getObjectFromItem(previousItem);
    SceneObject* current = getObjectFromItem(tree->currentItem());

    emit selectionChanged(previous, current);

    previousItem = tree->currentItem();
}

SceneObject *HierarchyWidget::getObjectFromItem(QTreeWidgetItem *item) {
    if (!item) return nullptr;
    QVariant var = item->data(0, Qt::UserRole);
    return static_cast<SceneObject*>(var.value<void*>());
}
