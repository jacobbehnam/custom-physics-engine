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
#include <QFrame>
#include <QHeaderView>
#include <QTableView>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <map>
#include <memory>

#include "HierarchyWidget.h"
#include "graph/FrameGraphPanel.h"
#include "inspector/InspectorWidget.h"
#include "SolverDialog.h"
#include "AppSettings.h"
#include "graphics/core/Camera.h"
#include "graphics/presets/ScenePresets.h"
#include "ui/settings/CameraSettingsGroup.h"
#include "ui/settings/DebugSettings.h"
#include "ui/settings/GraphicsSettings.h"
#include "ui/settings/SettingsDialog.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    AppSettings::getInstance().registerGroup<CameraSettingsGroup>();
    AppSettings::getInstance().registerGroup<DebugSettings>();
    AppSettings::getInstance().registerGroup<GraphicsSettings>();

    glWindow = new OpenGLWindow(this);

    setWindowTitle("Physics Engine");

    setCentralWidget(glWindow);
    glWindow->setFocus();

    fpsLabel = new QLabel(this);
    fpsLabel->setText("FPS: 0.0");
    statusBar()->addPermanentWidget(fpsLabel);

    connect(glWindow, &OpenGLWindow::fpsUpdated, this, [this](double fps) {
        fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
        updateStatusPanel();
    });

    connect(glWindow, &OpenGLWindow::glInitialized, this, &MainWindow::onGLInitialized);
}

void MainWindow::onGLInitialized() {
    auto scene = std::make_unique<Scene>(glWindow);
    Scene* scenePtr = scene.get();
    sceneManager = std::make_unique<SceneManager>(glWindow, scenePtr);
    glWindow->setScene(std::move(scene));
    glWindow->setSceneManager(sceneManager.get());
    setupMenuBar();
    setupDockWidgets();
    sceneManager->defaultSetup();
    loadAppSettings();
    connect(sceneManager.get(), &SceneManager::contextMenuRequested, this, &MainWindow::showObjectContextMenu);
}

void MainWindow::setupDockWidgets() {
    auto* infoDock = new QDockWidget(tr("Scene Info"), this);
    infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    infoDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);

    auto* infoPanel = new QWidget(infoDock);
    auto* infoLayout = new QFormLayout(infoPanel);
    infoLayout->setContentsMargins(8, 8, 8, 8);
    infoLayout->setSpacing(6);

    cameraPositionLabel = new QLabel("0, 0, 0", infoPanel);
    cameraPositionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    selectedObjectLabel = new QLabel("None", infoPanel);
    selectedObjectPositionLabel = new QLabel("-", infoPanel);
    selectedObjectPositionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    selectedObjectDistanceLabel = new QLabel("-", infoPanel);
    simulationStateLabel = new QLabel("Paused", infoPanel);
    renderClockStateLabel = new QLabel("Idle", infoPanel);
    cameraFollowLabel = new QLabel("Off", infoPanel);

    infoLayout->addRow("Camera position", cameraPositionLabel);
    infoLayout->addRow("Selected", selectedObjectLabel);
    infoLayout->addRow("Selected position", selectedObjectPositionLabel);
    infoLayout->addRow("Camera distance", selectedObjectDistanceLabel);
    infoLayout->addRow("Camera follow", cameraFollowLabel);
    infoLayout->addRow("Physics", simulationStateLabel);
    infoLayout->addRow("Render clock", renderClockStateLabel);

    auto* sceneInfoScrollArea = new QScrollArea(infoDock);
    sceneInfoScrollArea->setWidgetResizable(true);
    sceneInfoScrollArea->setFrameShape(QFrame::NoFrame);
    sceneInfoScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sceneInfoScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sceneInfoScrollArea->setWidget(infoPanel);

    infoDock->setWidget(sceneInfoScrollArea);
    infoDock->setMinimumHeight(80);
    infoDock->resize(300, 80);
    addDockWidget(Qt::LeftDockWidgetArea, infoDock);
    viewMenu->addAction(infoDock->toggleViewAction());

    auto* hierarchyDock = new QDockWidget(tr("Objects"), this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    hierarchy = new HierarchyWidget(this);
    hierarchyDock->setWidget(hierarchy);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);
    splitDockWidget(infoDock, hierarchyDock, Qt::Vertical);
    resizeDocks({infoDock, hierarchyDock}, {120, 600}, Qt::Vertical);
    viewMenu->addAction(hierarchyDock->toggleViewAction());

    connect(hierarchy, &HierarchyWidget::selectionChanged, this, &MainWindow::onHierarchySelectionChanged);
    connect(hierarchy, &HierarchyWidget::focusObjectRequested, this, [this](SceneObject* obj) {
        sceneManager->focusObject(obj);
        glWindow->setFocus();
    });
    connect(hierarchy, &HierarchyWidget::followObjectRequested, this, [this](SceneObject* obj) {
        sceneManager->setCameraTarget(obj);
        updateStatusPanel();
        glWindow->setFocus();
    });
    connect(hierarchy, &HierarchyWidget::clearCameraFollowRequested, this, [this]() {
        sceneManager->clearCameraTarget();
        updateStatusPanel();
        glWindow->setFocus();
    });
    connect(hierarchy, &HierarchyWidget::createObjectRequested, this, [this](const CreationOptions& options) {
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
    connect(sceneManager.get(), &SceneManager::objectAdded, this, [this](SceneObject* obj) { hierarchy->addObject(obj); inspector->unloadObject(); });
    connect(sceneManager.get(), &SceneManager::objectRemoved, this, [this](SceneObject* obj) {
        if (selectedInfoObject == obj)
            selectedInfoObject = nullptr;
        hierarchy->removeObject(obj);
        inspector->unloadObject();
        updateStatusPanel();
    });
    connect(sceneManager.get(), &SceneManager::objectRenamed, this, [this](SceneObject* obj, const QString& newName) { hierarchy->setObjectName(obj, newName); });
    connect(sceneManager.get(), &SceneManager::selectedItem, hierarchy, &HierarchyWidget::selectObject);

    inspector = new InspectorWidget(sceneManager.get(), this);
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
    viewMenu->addAction(inspectorDock->toggleViewAction());

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
    viewMenu->addAction(historyDock->toggleViewAction());
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

void MainWindow::setupPresetMenu() {
    QMenu* presetMenu = menuBar()->addMenu("Presets");
    std::map<QString, QMenu*> categoryMenus;

    for (const auto& preset : ScenePresets::all()) {
        const QString category = QString::fromUtf8(preset.category);
        QMenu*& categoryMenu = categoryMenus[category];
        if (!categoryMenu) {
            categoryMenu = presetMenu->addMenu(category);
        }

        QAction* action = categoryMenu->addAction(QString::fromUtf8(preset.name));
        action->setToolTip(QString::fromUtf8(preset.description));
        const ScenePresets::PresetDescriptor* presetPtr = &preset;
        connect(action, &QAction::triggered, this, [this, presetPtr]() {
            if (sceneManager->loadPreset(*presetPtr)) {
                snapshotModel->setSnapshots({});
                frameGraphPanel->clear();
                updateStatusPanel();
                statusBar()->showMessage(QString("Loaded preset: %1").arg(QString::fromUtf8(presetPtr->name)), 3000);
            } else {
                statusBar()->showMessage(QString("Failed to load preset: %1").arg(QString::fromUtf8(presetPtr->name)), 3000);
            }
        });
    }
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
    MainWindow::setupPresetMenu();
    viewMenu = menuBar()->addMenu("View");
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
    updateStatusPanel();
}

void MainWindow::updateStatusPanel() {
    if (!sceneManager || !sceneManager->scene || !sceneManager->scene->getCamera())
        return;

    const glm::vec3 pos = sceneManager->scene->getCamera()->position;
    cameraPositionLabel->setText(QString("x %1, y %2, z %3")
        .arg(pos.x, 0, 'g', 6)
        .arg(pos.y, 0, 'g', 6)
        .arg(pos.z, 0, 'g', 6));

    if (selectedInfoObject) {
        const glm::vec3 selectedPos = selectedInfoObject->getPosition();
        selectedObjectLabel->setText(QString::fromStdString(selectedInfoObject->getName()));
        selectedObjectPositionLabel->setText(QString("x %1, y %2, z %3")
            .arg(selectedPos.x, 0, 'g', 6)
            .arg(selectedPos.y, 0, 'g', 6)
            .arg(selectedPos.z, 0, 'g', 6));
        selectedObjectDistanceLabel->setText(QString("%1 m").arg(glm::distance(pos, selectedPos), 0, 'g', 6));
    } else {
        selectedObjectLabel->setText("None");
        selectedObjectPositionLabel->setText("-");
        selectedObjectDistanceLabel->setText("-");
    }

    simulationStateLabel->setText(sceneManager->isPhysicsRunning() ? QString("Running") : QString("Paused"));
    renderClockStateLabel->setText(glWindow->isRenderClockRunning() ? QString("Running") : QString("Idle"));
    if (const SceneObject* followed = sceneManager->getCameraTarget()) {
        cameraFollowLabel->setText(QString::fromStdString(followed->getName()));
    } else {
        cameraFollowLabel->setText("Off");
    }
}

void MainWindow::showObjectContextMenu(const QPoint &pos, SceneObject *obj) {
    if (!obj) return;

    QMenu contextMenu;
    QAction* solveAction = contextMenu.addAction("Open Solver...");
    QAction* followAction = contextMenu.addAction("Follow Camera");
    QAction* clearFollowAction = contextMenu.addAction("Stop Camera Follow");

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
    connect(followAction, &QAction::triggered, [this, obj]() {
        sceneManager->setCameraTarget(obj);
        updateStatusPanel();
    });
    connect(clearFollowAction, &QAction::triggered, [this]() {
        sceneManager->clearCameraTarget();
        updateStatusPanel();
    });

    contextMenu.exec(pos);
}
void MainWindow::onHierarchySelectionChanged(SceneObject *previous, SceneObject *current) {
    selectedInfoObject = current;
    updateStatusPanel();

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
