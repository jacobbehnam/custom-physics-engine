#include "MainWindow.h"
#include "OpenGLWindow.h"
#include <QDockWidget>
#include <QTreeWidget>
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
    fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1)); });

    connect(glWindow, &OpenGLWindow::glInitialized, this, [=]() {
        Scene* scene = new Scene(glWindow);
        sceneManager = new SceneManager(scene);
        glWindow->setScene(scene);
    });

    setupDockWidgets();
}

void MainWindow::setupDockWidgets() {
    auto* hierarchyDock = new QDockWidget(tr("Objects"), this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    auto* inspectorDock = new QDockWidget(tr("Inspector"), this);
    inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, inspectorDock);

}


MainWindow::~MainWindow() {
    // automatically handled
}
