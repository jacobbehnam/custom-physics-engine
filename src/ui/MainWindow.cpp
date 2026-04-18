#include "MainWindow.h"

#include <iostream>

#include "OpenGLWindow.h"
#include <QDockWidget>
#include <QHeaderView>
#include <QStatusBar>
#include <QLineEdit>
#include <QScrollArea>
#include <QMenuBar>
#include <QFileDialog>
#include <QDialog>
#include <QTabWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QDir> 
#include <QSettings>
#include <QTabWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <array>

#include "HierarchyWidget.h"
#include "FrameGraphWidget.h"
#include "inspector/InspectorWidget.h"
#include "SolverDialog.h"
#include "AppSettings.h"
#include "graphics/core/Camera.h"

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

    auto* graphTab = new QWidget(tabs);
    auto* graphTabLayout = new QVBoxLayout(graphTab);
    graphTabLayout->setContentsMargins(0, 0, 0, 0);

    auto* graphScrollArea = new QScrollArea(graphTab);
    graphScrollArea->setWidgetResizable(true);
    auto* graphContainer = new QWidget(graphScrollArea);
    frameGraphContainer = graphContainer;
    frameGraphLayout = new QGridLayout(graphContainer);
    frameGraphLayout->setContentsMargins(8, 8, 8, 8);
    frameGraphLayout->setSpacing(8);
    frameGraphLayout->setAlignment(Qt::AlignTop);

    const std::array<FrameGraphWidget::Metric, 6> metrics = {
        FrameGraphWidget::Metric::PositionX,
        FrameGraphWidget::Metric::PositionY,
        FrameGraphWidget::Metric::PositionZ,
        FrameGraphWidget::Metric::VelocityX,
        FrameGraphWidget::Metric::VelocityY,
        FrameGraphWidget::Metric::VelocityZ,
    };

    frameGraphs.reserve(metrics.size());
    for (FrameGraphWidget::Metric metric : metrics) {
        auto* graph = new FrameGraphWidget(graphContainer);
        graph->setMetric(metric);
        graph->setSelectorVisible(false);
        frameGraphs.push_back(graph);
    }

    relayoutFrameGraphs();

    graphScrollArea->setWidget(graphContainer);
    graphTabLayout->addWidget(graphScrollArea);
    tabs->addTab(graphTab, tr("Graphs"));

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
        QDialog dialog(this);
        dialog.setWindowTitle("Settings");
        dialog.resize(300, 200);
        QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
        QTabWidget* tabWidget = new QTabWidget(&dialog);
        
        QWidget* cameraTab = new QWidget();
        QFormLayout* cameraLayout = new QFormLayout(cameraTab);
        Camera* cam = sceneManager->scene->getCamera();
        const CameraSettings& currentSettings = cam->getSettings();

        // Mouse Sensitivity
        QDoubleSpinBox* sensBox = new QDoubleSpinBox();
        sensBox->setRange(0.01, 2.0);
        sensBox->setSingleStep(0.01);
        sensBox->setValue(currentSettings.mouseSensitivity);
        
        // Movement Speed
        QDoubleSpinBox* speedBox = new QDoubleSpinBox();
        speedBox->setRange(0.1, 100.0);
        speedBox->setSingleStep(0.5);
        speedBox->setValue(currentSettings.movementSpeed);
        
        // FOV
        QDoubleSpinBox* fovBox = new QDoubleSpinBox();
        fovBox->setRange(10.0, 120.0);
        fovBox->setSingleStep(1.0);
        fovBox->setValue(currentSettings.fov);

        cameraLayout->addRow("Mouse Sensitivity:", sensBox);
        cameraLayout->addRow("Movement Speed:", speedBox);
        cameraLayout->addRow("Field of View (FOV):", fovBox);
        tabWidget->addTab(cameraTab, "Camera");
        
        mainLayout->addWidget(tabWidget);
        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        mainLayout->addWidget(buttonBox);
        connect(buttonBox, &QDialogButtonBox::accepted, [&, this]() {
            saveCameraSettings({
                .movementSpeed = static_cast<float>(speedBox->value()),
                .mouseSensitivity = static_cast<float>(sensBox->value()),
                .fov = static_cast<float>(fovBox->value()),
            });
            dialog.accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        dialog.exec();
    });
}

void MainWindow::setupMenuBar() {
    MainWindow::setupFileMenu();
    MainWindow::setupSettingMenu();
}

void MainWindow::loadAppSettings() {
    if (!sceneManager || !sceneManager->scene) {
        return;
    }

    Camera* camera = sceneManager->scene->getCamera();
    if (!camera) {
        return;
    }

    QSettings settings;
    const AppSettings appSettings = AppSettings::load(settings);
    camera->setSettings(appSettings.camera);
}

void MainWindow::saveCameraSettings(const CameraSettings& cameraSettings) {
    if (!sceneManager || !sceneManager->scene) {
        return;
    }

    Camera* camera = sceneManager->scene->getCamera();
    if (!camera) {
        return;
    }

    camera->setSettings(cameraSettings);

    QSettings qSettings;
    AppSettings appSettings = AppSettings::load(qSettings);
    appSettings.camera = cameraSettings;
    appSettings.save(qSettings);
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
void MainWindow::clearFrameGraph() {
    snapshotModel->setSnapshots({});
    for (FrameGraphWidget* graph : frameGraphs) {
        graph->clear();
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    
    relayoutFrameGraphs(); 
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
            const std::vector<ObjectSnapshot> snapshots = selectedBody->getAllFrames(BodyLock::LOCK);
            snapshotModel->setSnapshots(snapshots);
            for (FrameGraphWidget* graph : frameGraphs) {
                graph->setSnapshots(snapshots);
            }
        } else {
            MainWindow::clearFrameGraph();
        }
    } else {
        inspector->unloadObject();
        MainWindow::clearFrameGraph();
    }
    relayoutFrameGraphs();
}

// Auto relayout number of graphs on a row
void MainWindow::relayoutFrameGraphs() {
    if (!frameGraphLayout) {
        return;
    }
    const int maxTracks = static_cast<int>(frameGraphs.size());
    for (int index = 0; index < maxTracks; ++index) {
        frameGraphLayout->setColumnStretch(index, 0);
        frameGraphLayout->setColumnMinimumWidth(index, 0);
        frameGraphLayout->setRowStretch(index, 0);
        frameGraphLayout->setRowMinimumHeight(index, 0);
    }

    QWidget* parentWidget = frameGraphLayout->parentWidget();
    const int availableWidth = parentWidget
        ? parentWidget->contentsRect().width() - frameGraphLayout->contentsMargins().left() - frameGraphLayout->contentsMargins().right()
        : 0;
    const int minimumCardWidth = 240;
    const int spacing = frameGraphLayout->horizontalSpacing();
    const int columns = std::max(1, (availableWidth + spacing) / (minimumCardWidth + spacing));
    if (columns == frameGraphColumns && frameGraphLayout->count() == static_cast<int>(frameGraphs.size())) {
        return;
    }
    frameGraphColumns = columns;
    for (FrameGraphWidget* graph : frameGraphs) {
        frameGraphLayout->removeWidget(graph);
    }


    for (int i = 0; i < static_cast<int>(frameGraphs.size()); ++i) {
        const int row = i / columns;
        const int column = i % columns;
        frameGraphLayout->addWidget(frameGraphs[i], row, column);
    }

    for (int column = 0; column < columns; ++column) {
        frameGraphLayout->setColumnStretch(column, 1);
    }

    frameGraphLayout->invalidate();
    if (parentWidget) {
        parentWidget->updateGeometry();
    }
}

MainWindow::~MainWindow() {}
