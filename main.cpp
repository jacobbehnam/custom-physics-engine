#include <QApplication>
#include <ui/MainWindow.h>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.resize(800, 600); // or whatever size you want
    mainWindow.show();

    return QApplication::exec();
}