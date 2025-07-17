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

class OpenGLWindow;


class Scene {
public:
    Scene(OpenGLWindow* win);
    ~Scene() = default;
    void draw(const std::unordered_set<uint32_t>& hoverIDs, const std::unordered_set<uint32_t>& selectIDs);
    void update(float dt);

    template<typename T>
    void addObject(T* obj);
    void addDrawable(IDrawable* obj) { drawableObjects.push_back(obj); }
    void removeDrawable(IDrawable* obj);
    void deleteSceneObject(SceneObject* obj);

    uint32_t allocateObjectID();
    void freeObjectID(uint32_t objID);

    void setHoveredFor(SceneObject* obj, bool flag);

    Camera* getCamera();
    OpenGLWindow* getWindow() { return window; }

    std::unique_ptr<Physics::PhysicsSystem> physicsSystem; // TODO: move
private:
    MathUtils::Ray getMouseRay();

    OpenGLWindow* window;
    Camera camera;
    std::vector<IDrawable*> drawableObjects;

    Shader* basicShader;

    UniformBuffer cameraUBO;
    UniformBuffer hoverUBO;
    UniformBuffer selectUBO;

    uint32_t nextID = 0;
    std::deque<uint32_t> freeIDs;
    std::unordered_set<int32_t> hoveredIDs;
};