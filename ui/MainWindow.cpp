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
    Scene* scene = new Scene(glWindow);
    sceneManager = new SceneManager(scene);
    glWindow->setScene(scene);
    setupDockWidgets();
    sceneManager->defaultSetup();
}

void MainWindow::setupDockWidgets() {
    auto* hierarchyDock = new QDockWidget(tr("Objects"), this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    hierarchyTree = new QTreeWidget(hierarchyDock);  // store as member
    hierarchyTree->setHeaderLabels({ "Name", "Type" });
    hierarchyDock->setWidget(hierarchyTree);

    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    connect(sceneManager, &SceneManager::objectAdded, this, [=]() {
        auto* item = new QTreeWidgetItem();
        item->setText(0, QString::fromStdString("Cube"));
        item->setText(1, QString::fromStdString("SceneObject"));
        hierarchyTree->addTopLevelItem(item);
    });

    auto* inspectorDock = new QDockWidget(tr("Inspector"), this);
    inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, inspectorDock);

}


MainWindow::~MainWindow() {
    // automatically handled
}
