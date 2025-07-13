#include "MainWindow.h"
#include "OpenGLWindow.h"
#include <QAction>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), glWindow(new OpenGLWindow(this)) {
    setWindowTitle("Cool Stuff");

    setCentralWidget(glWindow);
    glWindow->setFocus();

    fpsLabel = new QLabel(this);
    fpsLabel->setText("FPS: 0.0");

    statusBar()->addPermanentWidget(fpsLabel);

    connect(glWindow, &OpenGLWindow::fpsUpdated, this, [this](double fps) {
    fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1)); });
}

MainWindow::~MainWindow() {
    // automatically handled
}
