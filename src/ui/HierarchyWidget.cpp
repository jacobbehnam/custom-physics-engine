#include "HierarchyWidget.h"

#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QHeaderView>
#include <QToolButton>

#include "physics/PointMass.h"

HierarchyWidget::HierarchyWidget(QWidget* parent) : QWidget(parent) {
    tree = new QTreeWidget(this);
    tree->setHeaderLabels({ "Name", "Type" });
    tree->setContextMenuPolicy(Qt::CustomContextMenu);

    QToolButton* addButton = new QToolButton(this);
    addButton->setText("Add");
    addButton->setAutoRaise(true);
    addButton->setPopupMode(QToolButton::InstantPopup);

    QMenu* addMenu = new QMenu(addButton);
    QAction* addSceneObject = addMenu->addAction("Empty Scene Object");
    connect(addSceneObject, &QAction::triggered, this, [this]() {
        emit createObjectRequested({ObjectOptions()});
    });
    QAction* addPointMass = addMenu->addAction("Point Mass");
    connect(addPointMass, &QAction::triggered, this, [this]() {
        emit createObjectRequested({PointMassOptions()});
    });
    QAction* addRigidBody = addMenu->addAction("Rigid Body");
    connect(addRigidBody, &QAction::triggered, this, [this]() {
        emit createObjectRequested({RigidBodyOptions::Box({})}); // TODO: should be a sphere but I haven't implemented a sphere collider yet
    });
    addButton->setMenu(addMenu);

    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->addWidget(addButton);
    headerLayout->addStretch();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(0);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(headerLayout);
    layout->addWidget(tree);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setLayout(layout);

    connect(tree, &QTreeWidget::itemSelectionChanged, this, &HierarchyWidget::onItemSelectionChanged);
    connect(tree, &QTreeWidget::customContextMenuRequested, this, &HierarchyWidget::showContextMenu);
}

void HierarchyWidget::showContextMenu(const QPoint& pos) {
    QMenu contextMenu(this);

    QAction* addPmAction = contextMenu.addAction("Add Point Mass");

    // connect(addPmAction, &QAction::triggered, [this]() {
    //     emit createObjectRequested("Point Mass");
    // });

    contextMenu.exec(tree->mapToGlobal(pos));
}

QString HierarchyWidget::typeFor(SceneObject* obj) {
    if (!obj->getPhysicsBody())
        return "Scene Object";

    if (dynamic_cast<Physics::PointMass*>(obj->getPhysicsBody()))
        return "Point Mass";

    if (dynamic_cast<Physics::RigidBody*>(obj->getPhysicsBody()))
        return "Rigid Body";

    return "Physics Object";
}

void HierarchyWidget::addObject(SceneObject* obj) {
    auto* item = new QTreeWidgetItem();
    item->setText(0, QString::fromStdString(obj->getName()));
    item->setText(1, typeFor(obj));
    item->setData(0, Qt::UserRole, QVariant::fromValue<void*>(obj));
    tree->addTopLevelItem(item);
}

void HierarchyWidget::removeObject(SceneObject *obj) {
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = tree->topLevelItem(i);

        if (getObjectFromItem(item) == obj) {
            if (item == previousItem)
                previousItem = nullptr;

            delete tree->takeTopLevelItem(i);
            break;
        }
    }
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
