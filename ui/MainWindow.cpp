#include "MainWindow.h"

#include <iostream>

#include "OpenGLWindow.h"
#include <QDockWidget>
#include <QStatusBar>

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

    auto* inspectorDock = new QDockWidget(tr("Inspector"), this);
    inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, inspectorDock);

}

void MainWindow::onHierarchyItemSelected() {
    QTreeWidgetItem* selectedItem = hierarchyTree->currentItem();
    if (selectedItem) {
        QString name = selectedItem->text(0);
        qDebug() << "Selected:" << name;

        QVariant var = selectedItem->data(0, Qt::UserRole);
        void* ptr = var.value<void*>();
        SceneObject* sceneObject = static_cast<SceneObject*>(ptr);
        scene->setHoveredFor(sceneObject, true);
    }
}

MainWindow::~MainWindow() {
    // automatically handled
}
