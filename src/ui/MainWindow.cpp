#include "MainWindow.h"

#include <iostream>

#include "OpenGLWindow.h"
#include <QDockWidget>
#include <QStatusBar>
#include <QLineEdit>
#include <QScrollArea>
#include <QMenuBar>

#include "HierarchyWidget.h"
#include "InspectorWidget.h"
#include "SolverDialog.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    glWindow = new OpenGLWindow(nullptr, this);

    setWindowTitle("Physics Engine");

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
    setupMenuBar();
    setupDockWidgets();
    sceneManager->defaultSetup();
    connect(sceneManager, &SceneManager::contextMenuRequested, this, &MainWindow::showObjectContextMenu);
}

void MainWindow::setupDockWidgets() {
    auto* hierarchyDock = new QDockWidget(tr("Objects"), this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    hierarchy = new HierarchyWidget(this);
    hierarchyDock->setWidget(hierarchy);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    connect(hierarchy, &HierarchyWidget::selectionChanged, this, &MainWindow::onHierarchySelectionChanged);
    connect(hierarchy, &HierarchyWidget::createObjectRequested, this, [=](const CreationOptions& options) {
        SceneObject* createdObj = sceneManager->createObject("prim_sphere", ResourceManager::getShader("basic"), options);
        hierarchy->selectObject(createdObj);
    });
    connect(hierarchy, &HierarchyWidget::renameObjectRequested, this, [this](SceneObject* obj, const QString& requestedName) {
        std::string requested = requestedName.toStdString();
        if (requested.empty()) {
            hierarchy->setObjectName(obj, obj->getName());
            return;
        }
        std::string finalName = requested;

        if (!sceneManager->isNameUnique(requested, obj)) {
            finalName = sceneManager->makeUniqueName(requested);
        }

        sceneManager->setObjectName(obj, finalName);
        hierarchy->setObjectName(obj, finalName);
    });
    connect(sceneManager, &SceneManager::objectAdded, this, [=](SceneObject* obj) { hierarchy->addObject(obj); inspector->unloadObject(true); });
    connect(sceneManager, &SceneManager::objectRemoved, this, [=](SceneObject* obj) { hierarchy->removeObject(obj); inspector->unloadObject(true); });
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

    auto* tableDock = new QDockWidget(tr("Frame History"), this);
    tableDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    auto* tableView = new QTableView(this);
    snapshotModel = new SnapshotTableModel(this);
    tableView->setModel(snapshotModel);
    tableDock->setWidget(tableView);
    addDockWidget(Qt::RightDockWidgetArea, tableDock);
}

void MainWindow::setupMenuBar() {
    // File Menu
    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *saveAction = new QAction("Save", this);
    fileMenu->addAction(saveAction);
    QAction *loadAction = new QAction("Load", this);
    fileMenu->addAction(loadAction);

    connect(saveAction, &QAction::triggered, this, [this](){
        if (sceneManager->saveScene("scene.json"))
            std::cout << "Save Success!" << std::endl;
        else
            std::cout << "Save Failed!" << std::endl;
    });
    connect(loadAction, &QAction::triggered, this, [this](){
        if (sceneManager->loadScene("scene.json"))
            std::cout << "Load Success!" << std::endl;
        else
            std::cout << "Load Failed!" << std::endl;
    });
}

void MainWindow::showObjectContextMenu(const QPoint &pos, SceneObject *obj) {
    if (!obj) return;

    QMenu contextMenu;
    QAction* solveAction = contextMenu.addAction("Open Solver...");

    connect(solveAction, &QAction::triggered, [this, obj]() {
        Physics::PhysicsBody* body = obj->getPhysicsBody();

        if (body) {
            const ProblemRouter *router = sceneManager->physicsSystem->getRouter();
            // Create and show the dialog
            SolverDialog dialog(router, body, this);
            if (dialog.exec() == QDialog::Accepted) {
                auto knowns = dialog.getCollectedKnowns();
                std::string unknown = dialog.getTargetUnknown();

                sceneManager->physicsSystem->solveProblem(body, knowns, unknown);
            }
        } else {
            qDebug() << "Selected object has no physics body attached.";
        }
    });

    contextMenu.exec(pos);
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
