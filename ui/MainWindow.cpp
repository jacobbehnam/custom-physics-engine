#include "MainWindow.h"
#include "OpenGLWindow.h"
#include <QAction>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), glWindow(new OpenGLWindow(this)) {
    setWindowTitle("My App with OpenGL");

    // Set OpenGLWindow as the central widget
    setCentralWidget(glWindow);
    glWindow->setFocus();
}

MainWindow::~MainWindow() {
    // automatically handled
}
