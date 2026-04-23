#include "MainWindow.h"

#include <iostream>

#include "OpenGLWindow.h"
#include <QDockWidget>
#include <QStatusBar>
#include <QLineEdit>
#include <QScrollArea>
#include <QMenuBar>
#include <QFileDialog>
#include <QDir> 
#include <QHeaderView>
#include <QTableView>
#include <QTabWidget>
#include <QVBoxLayout>

#include "HierarchyWidget.h"
#include "graph/FrameGraphPanel.h"
#include "inspector/InspectorWidget.h"
#include "SolverDialog.h"
#include "AppSettings.h"
#include "graphics/core/Camera.h"
#include "ui/settings/CameraSettingsGroup.h"
#include "ui/settings/DebugSettings.h"
#include "ui/settings/SettingsDialog.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    AppSettings::getInstance().registerGroup<CameraSettingsGroup>();
    AppSettings::getInstance().registerGroup<DebugSettings>();

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
    loadAppSettings();
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
            hierarchy->setObjectName(obj, obj->getName().data());
            return;
        }
        std::string finalName = requested;

        if (!sceneManager->isNameUnique(requested, obj)) {
            finalName = sceneManager->makeUniqueName(requested);
        }

        sceneManager->setObjectName(obj, finalName);
    });
    connect(hierarchy, &HierarchyWidget::deleteObjectRequested, this, [this](SceneObject* obj) {
        sceneManager->deleteObject(obj);
    });
    connect(sceneManager, &SceneManager::objectAdded, this, [=](SceneObject* obj) { hierarchy->addObject(obj); inspector->unloadObject(); });
    connect(sceneManager, &SceneManager::objectRemoved, this, [=](SceneObject* obj) { hierarchy->removeObject(obj); inspector->unloadObject(); });
    connect(sceneManager, &SceneManager::objectRenamed, this, [=](SceneObject* obj, const QString& newName) { hierarchy->setObjectName(obj, newName); });
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

    auto* historyDock = new QDockWidget(tr("Frame History"), this);
    historyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* tabs = new QTabWidget(historyDock);

    auto* tableView = new QTableView(tabs);
    tableView->horizontalHeader()->setStretchLastSection(true);
    snapshotModel = new SnapshotTableModel(this);

    tableView->setModel(snapshotModel);
    tabs->addTab(tableView, tr("History"));

    frameGraphPanel = new FrameGraphPanel(tabs);
    tabs->addTab(frameGraphPanel, tr("Graphs"));

    historyDock->setWidget(tabs);
    addDockWidget(Qt::RightDockWidgetArea, historyDock);
}

void MainWindow::setupFileMenu() {
    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *saveAction = new QAction("Save", this);
    fileMenu->addAction(saveAction);
    QAction *saveAsAction = new QAction("Save As", this);
    fileMenu->addAction(saveAsAction);
    QAction *loadAction = new QAction("Load", this);
    fileMenu->addAction(loadAction);
    QAction *loadFromAction = new QAction("Load From", this);
    fileMenu->addAction(loadFromAction);

    connect(saveAction, &QAction::triggered, this, [this](){
        if (sceneManager->saveScene("scene.json")) {
            std::cout << "Save Success!" << std::endl;
            statusBar()->showMessage("Scene saved to scene.json", 3000);
        } else {
            std::cout << "Save Failed!" << std::endl;
            statusBar()->showMessage("Failed to save scene to scene.json", 3000);
        }
    });
    connect(loadAction, &QAction::triggered, this, [this](){
        if (sceneManager->loadScene("scene.json")) {
            std::cout << "Load Success!" << std::endl;
            statusBar()->showMessage("Scene loaded from scene.json", 3000);
        } else {
            std::cout << "Load Failed!" << std::endl;
            statusBar()->showMessage("Failed to load scene from scene.json", 3000);
        }
    });
    connect(saveAsAction, &QAction::triggered, this, [this](){
        QFileDialog dialog(this, "Save Scene", QDir::currentPath(), "JSON Files (*.json)");
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setDefaultSuffix("json");

        if (dialog.exec() == QDialog::Accepted) {
            const QString fileName = dialog.selectedFiles().value(0);
            if (sceneManager->saveScene(fileName)) {
                std::cout << "Save Success!" << std::endl;
                statusBar()->showMessage(QString("Scene saved to %1").arg(fileName), 3000);
            }
            else {
                std::cout << "Save Failed!" << std::endl;
                statusBar()->showMessage(QString("Failed to save scene to %1").arg(fileName), 3000);
            }
        }
    });
    connect(loadFromAction, &QAction::triggered, this, [this](){
        QFileDialog dialog(this, "Load Scene", QDir::currentPath(), "JSON Files (*.json)");
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFileMode(QFileDialog::ExistingFile);

        if (dialog.exec() == QDialog::Accepted) {
            const QString fileName = dialog.selectedFiles().value(0);
            if (sceneManager->loadScene(fileName)) {
                std::cout << "Load Success!" << std::endl;
                statusBar()->showMessage(QString("Scene loaded from %1").arg(fileName), 3000);
            } else {
                std::cout << "Load Failed!" << std::endl;
                statusBar()->showMessage(QString("Failed to load scene from %1").arg(fileName), 3000);
            }
        }
    });
}

void MainWindow::setupSettingMenu() {
    QMenu *settingMenu = menuBar()->addMenu("Settings");
    QAction *preferencesAction = new QAction("Preferences", this);
    settingMenu->addAction(preferencesAction);
    connect(preferencesAction, &QAction::triggered, this, [this]() {
        SettingsDialog dialog(this);
        connect(&dialog, &SettingsDialog::settingsSaved, this, [this]() {
            // Push saved settings down to Camera and debug drawables
            auto& camGroup = AppSettings::getInstance().getGroup<CameraSettingsGroup>();
            Camera* camera = sceneManager->scene->getCamera();
            camera->movementSpeed = camGroup.movementSpeed;
            camera->mouseSensitivity = camGroup.mouseSensitivity;
            camera->fov = camGroup.fov;

            sceneManager->applyDebugSettings();
        });
        dialog.exec();
    });
}

void MainWindow::setupMenuBar() {
    MainWindow::setupFileMenu();
    MainWindow::setupSettingMenu();
}

void MainWindow::loadAppSettings() {
    QSettings settings;
    AppSettings::getInstance().load(settings);

    // Push loaded settings down to Camera
    auto& camGroup = AppSettings::getInstance().getGroup<CameraSettingsGroup>();
    Camera* camera = sceneManager->scene->getCamera();
    camera->movementSpeed = camGroup.movementSpeed;
    camera->mouseSensitivity = camGroup.mouseSensitivity;
    camera->fov = camGroup.fov;

    // Push loaded settings down to debug drawables
    sceneManager->applyDebugSettings();
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

        if (auto* selectedBody = current->getPhysicsBody()) {
            selectedBody->withFrames(BodyLock::LOCK, [this](const std::vector<ObjectSnapshot>& snapshots) {
                snapshotModel->setSnapshots(snapshots);
                frameGraphPanel->loadSnapshots(snapshots);
            });
        } else {
            snapshotModel->setSnapshots({});
            frameGraphPanel->clear();
        }
    } else {
        inspector->unloadObject();
        snapshotModel->setSnapshots({});
        frameGraphPanel->clear();
    }
}

MainWindow::~MainWindow() {}
