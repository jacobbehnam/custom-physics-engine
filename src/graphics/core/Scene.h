#pragma once
#include <graphics/core/Camera.h>
#include <graphics/components/Gizmo.h>
#include <math/MathUtils.h>
#include <graphics/core/UniformBuffer.h>
#include <vector>
#include <deque>
#include <unordered_set>

class Scene {
public:
    Scene(QOpenGLFunctions_4_5_Core* glFuncs);
    ~Scene() = default;
    void draw(const std::optional<std::vector<ObjectSnapshot>>& snapshots, const std::unordered_set<uint32_t>& hoverIDs, const std::unordered_set<uint32_t>& selectIDs);

    void addDrawable(IDrawable* drawable);
    void removeDrawable(IDrawable* drawable);

    uint32_t allocateObjectID();
    void freeObjectID(uint32_t objID);

    Camera* getCamera();
private:
    QOpenGLFunctions_4_5_Core* funcs;

    Camera camera;
    std::vector<IInstancedDrawable*> instancedDrawables;
    std::vector<ICustomDrawable*> customDrawables;

    Shader* basicShader;

    UniformBuffer cameraUBO;
    UniformBuffer hoverUBO;
    UniformBuffer selectUBO;

    uint32_t nextID = 0;
    std::deque<uint32_t> freeIDs;
};