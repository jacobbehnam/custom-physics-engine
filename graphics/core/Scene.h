#pragma once
#include <graphics/core/Camera.h>
#include <graphics/core/SceneObject.h>
#include <graphics/components/Gizmo.h>
#include <graphics/utils/MathUtils.h>
#include <graphics/core/UniformBuffer.h>
#include <vector>
#include <deque>
#include <unordered_set>
#include <GLFW/glfw3.h>
#include <QEvent>

class Scene {
public:
    Scene(QOpenGLFunctions_4_5_Core* glFuncs);
    ~Scene() = default;
    void draw(const std::optional<std::vector<ObjectSnapshot>>& snapshots, const std::unordered_set<uint32_t>& hoverIDs, const std::unordered_set<uint32_t>& selectIDs);

    void addDrawable(IDrawable* obj) { drawableObjects.push_back(obj); }
    void removeDrawable(IDrawable* obj);

    uint32_t allocateObjectID();
    void freeObjectID(uint32_t objID);

    Camera* getCamera();
private:
    QOpenGLFunctions_4_5_Core* funcs;

    Camera camera;
    std::vector<IDrawable*> drawableObjects;

    Shader* basicShader;

    UniformBuffer cameraUBO;
    UniformBuffer hoverUBO;
    UniformBuffer selectUBO;

    uint32_t nextID = 0;
    std::deque<uint32_t> freeIDs;
};