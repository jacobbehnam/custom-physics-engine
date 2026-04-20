#pragma once

#include <vector>
#include <QOpenGLFunctions_4_5_Core>
#include "graphics/components/Shader.h"
#include "graphics/components/Mesh.h"
#include "graphics/core/IDrawable.h"

class SceneManager;

class Colliders : public ICustomDrawable {
public:
    Colliders(SceneManager* sceneManager, QOpenGLFunctions_4_5_Core* glFuncs);
    ~Colliders() override = default;

    Shader* getShader() const override { return basicShader; }
    Mesh* getMesh() const override { return nullptr; }
    uint32_t getObjectID() const override { return 0; }

    void draw() const override;

    void setEnabled(bool value) { enabled = value; }

private:
    SceneManager* sceneManager;
    QOpenGLFunctions_4_5_Core* gl;
    Shader* basicShader = nullptr;
    Mesh* cubeMesh = nullptr;
    bool enabled = false;
};
