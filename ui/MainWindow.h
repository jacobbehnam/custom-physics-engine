#pragma once

#include <QMainWindow>
#include <QToolBar>

class OpenGLWindow;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    OpenGLWindow* glWindow;
};