#include <QApplication>
#include <ui/MainWindow.h>

#include "ui/OpenGLWindow.h"
#include "ui/RawInputFilter.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.resize(800, 600);
    mainWindow.show();

    // input gets handled by RawInputFilter before Qt handles it
    auto filter = new RawInputFilter([&](int dx, int dy){
    mainWindow.getGlWindow()->handleRawMouseDelta(dx, dy); });
    app.installNativeEventFilter(filter);

    return QApplication::exec();
}
