#include "MainWindow.h"

#include <iostream>

#include "OpenGLWindow.h"
#include <QDockWidget>
#include <QStatusBar>
#include <QLineEdit>
#include <QScrollArea>

#include "HierarchyWidget.h"
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
    auto* scene = new Scene(glWindow);
    sceneManager = new SceneManager(glWindow, scene);
    glWindow->setScene(scene);
    glWindow->setSceneManager(sceneManager);
    setupDockWidgets();
    sceneManager->defaultSetup();
}

void MainWindow::setupDockWidgets() {
    auto* hierarchyDock = new QDockWidget(tr("Objects"), this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    hierarchy = new HierarchyWidget(this);
    hierarchyDock->setWidget(hierarchy);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    connect(hierarchy, &HierarchyWidget::selectionChanged, this, &MainWindow::onHierarchySelectionChanged);
    connect(sceneManager, &SceneManager::objectAdded, this, [=](SceneObject* obj) { hierarchy->addObject(obj); });
    connect(sceneManager, &SceneManager::objectRemoved, this, [=](SceneObject* obj) { hierarchy->removeObject(obj); });
    connect(sceneManager, &SceneManager::selectedItem, hierarchy, &HierarchyWidget::selectObject);

    inspector = new InspectorWidget(sceneManager, this);
    auto* inspectorDock = new QDockWidget(tr("Inspector"), this);
    inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidget(inspector);
    scrollArea->setMinimumWidth(350);
    scrollArea->setMinimumHeight(200);

    inspectorDock->setWidget(scrollArea);
    addDockWidget(Qt::LeftDockWidgetArea, inspectorDock);

    auto* tableDock = new QDockWidget(tr("Bruh"), this);
    tableDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    auto* tableView = new QTableView(this);
    snapshotModel = new SnapshotTableModel(this);
    tableView->setModel(snapshotModel);
    tableDock->setWidget(tableView);
    addDockWidget(Qt::RightDockWidgetArea, tableDock);
}

void MainWindow::onHierarchySelectionChanged(SceneObject *previous, SceneObject *current) {
    if (previous) {
        sceneManager->setSelectFor(previous, false);
    }
    if (current) {
        sceneManager->setSelectFor(current, true);
        sceneManager->setGizmoFor(current, true);
        inspector->loadObject(current);
        if (auto* body = current->getPhysicsBody()) {
            std::vector<ObjectSnapshot> snaps = body->getAllFrames(BodyLock::LOCK);
            snapshotModel->setSnapshots(snaps);
        }
    } else {
        inspector->unloadObject();
    }
}

MainWindow::~MainWindow() {
    // automatically handled
}
