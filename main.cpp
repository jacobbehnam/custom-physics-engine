#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <graphics/components/Mesh.h>
#include <graphics/core/SceneObject.h>
#include <graphics/components/Shader.h>
#include "graphics/components/TranslateHandle.h"
#include <graphics/core/Scene.h>
#include "graphics/core/ResourceManager.h"
#include "physics/PhysicsSystem.h"
#include <QApplication>
#include <ui/OpenGLWindow.h>
#include <graphics/core/Scene.h>
#include <physics/PhysicsSystem.h>
#include <ui/MainWindow.h>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.resize(800, 600); // or whatever size you want
    mainWindow.show();

    return QApplication::exec();
}