#pragma once

#include <vector>
#include <QOpenGLFunctions_4_5_Core>
#include "graphics/components/Shader.h"
#include "graphics/core/SceneObject.h"
#include "graphics/core/IDrawable.h"

class SceneManager;

class PathTraces : public ICustomDrawable {
public:
    PathTraces(SceneManager* sceneManager, QOpenGLFunctions_4_5_Core* glFuncs);
    ~PathTraces() override;

    Shader* getShader() const override { return traceShader; }
    Mesh* getMesh() const override { return nullptr; }
    uint32_t getObjectID() const override { return 0; }

    void draw() const override;

    void setEnabled(bool value) { enabled = value; }
    void setTimeWindow(float value) { timeWindow = value; }

private:
    SceneManager* sceneManager;
    QOpenGLFunctions_4_5_Core* gl;
    GLuint vao = 0;
    GLuint vbo = 0;
    Shader* traceShader = nullptr;
    bool enabled = true;
    float timeWindow = 2.0f;
};