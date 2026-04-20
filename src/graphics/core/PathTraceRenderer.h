#pragma once

#include <vector>
#include <QOpenGLFunctions_4_5_Core>
#include "graphics/components/Shader.h"
#include "graphics/core/SceneObject.h"

class PathTraceRenderer {
public:
    PathTraceRenderer(QOpenGLFunctions_4_5_Core* glFuncs);
    ~PathTraceRenderer();

    void drawTrails(const std::vector<class SceneObject*>& objects, float timeWindow);

private:
    QOpenGLFunctions_4_5_Core* gl;
    GLuint vao = 0;
    GLuint vbo = 0;
    Shader* traceShader = nullptr;
};
