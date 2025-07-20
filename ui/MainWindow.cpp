#include "MainWindow.h"

#include <iostream>

#include "OpenGLWindow.h"
#include <QDockWidget>
#include <QStatusBar>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>

#include "InspectorWidget.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    glWindow = new OpenGLWindow(nullptr, this);

    setWindowTitle("Cool Stuff");

    setCentralWidget(glWindow);
    glWindow->setFocus();

    fpsLabel = new QLabel(this);
    fpsLabel->setText("FPS: 0.0");

    statusBar()->addPermanentWidget(fpsLabel);

    connect(glWindow, &OpenGLWindow::fpsUpdated, this, [this](double fps) {
        fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
    });

    connect(glWindow, &OpenGLWindow::glInitialized, this, &MainWindow::onGLInitialized);
}

void MainWindow::onGLInitialized() {
    scene = new Scene(glWindow);
    sceneManager = new SceneManager(scene);
    glWindow->setScene(scene);
    glWindow->setSceneManager(sceneManager);
    setupDockWidgets();
    sceneManager->defaultSetup();
}

void MainWindow::setupDockWidgets() {
    auto* hierarchyDock = new QDockWidget(tr("Objects"), this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    hierarchyTree = new QTreeWidget(hierarchyDock);  // store as member
    hierarchyTree->setHeaderLabels({ "Name", "Type" });
    hierarchyDock->setWidget(hierarchyTree);
    connect(hierarchyTree, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onHierarchyItemSelected);

    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    connect(sceneManager, &SceneManager::objectAdded, this, [=](SceneObject* obj) {
        auto* item = new QTreeWidgetItem();
        item->setText(0, QString::fromStdString("Cube"));
        item->setText(1, QString::fromStdString("SceneObject"));
        item->setData(0, Qt::UserRole, QVariant::fromValue<void*>(obj));
        hierarchyTree->addTopLevelItem(item);
    });
    connect(sceneManager, &SceneManager::selectedItem, this, &MainWindow::changeHierarchyItemSelected);

    inspector = new InspectorWidget(this);
    auto* inspectorDock = new QDockWidget(tr("Inspector"), this);
    inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    inspectorDock->setWidget(inspector);
    addDockWidget(Qt::LeftDockWidgetArea, inspectorDock);
}

void MainWindow::onHierarchyItemSelected() {
    QTreeWidgetItem* currentItem = hierarchyTree->currentItem();

    if (previousItem && previousItem != currentItem) {
        QVariant var = previousItem->data(0, Qt::UserRole);
        void* ptr = var.value<void*>();
        SceneObject* previousObject = static_cast<SceneObject*>(ptr);
        sceneManager->setSelectFor(previousObject, false);
    }

    if (currentItem) {
        QVariant var = currentItem->data(0, Qt::UserRole);
        void* ptr = var.value<void*>();
        SceneObject* currentObject = static_cast<SceneObject*>(ptr);
        sceneManager->setSelectFor(currentObject, true);
        sceneManager->setGizmoFor(currentObject, true);
        inspector->loadObject(currentObject);

    }

    previousItem = currentItem;
}

void MainWindow::changeHierarchyItemSelected(SceneObject* obj) {
    if (!obj) { // nullptr is the indicator for "deselect all items in hierarchy"
        hierarchyTree->setCurrentItem(nullptr);
        hierarchyTree->clearSelection();
        return;
    }

    for (int i = 0; i < hierarchyTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = hierarchyTree->topLevelItem(i);
        QVariant var = item->data(0, Qt::UserRole);
        void* ptr = var.value<void*>();
        if (ptr == obj) {
            hierarchyTree->setCurrentItem(item);
            // Above will then call onHierarchyItemSelected
            break;
        }
    }
}

MainWindow::~MainWindow() {
    // automatically handled
}
