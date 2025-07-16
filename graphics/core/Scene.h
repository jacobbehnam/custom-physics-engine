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
    void draw(const std::unordered_set<uint32_t>& hoverIDs);
    void update(float dt);

    template<typename T>
    void addObject(T* obj);
    void addDrawable(IDrawable* obj) { drawableObjects.push_back(obj); }
    void deleteSceneObject(SceneObject* obj);

    uint32_t allocateObjectID();
    void freeObjectID(uint32_t objID);

    void processInput(float dt);
    void handleMouseButton(Qt::MouseButton button, QEvent::Type eventType, Qt::KeyboardModifiers mods);

    void setGizmoFor(SceneObject* newTarget, bool redraw = false);
    void deleteGizmo();

    void setHoveredFor(SceneObject* obj, bool flag);

    Camera* getCamera();
private:
    MathUtils::Ray getMouseRay();

    OpenGLWindow* window;
    Camera camera;
    std::vector<IDrawable*> drawableObjects;
    std::vector<IPickable*> pickableObjects;
    Shader* basicShader;

    UniformBuffer cameraUBO;
    UniformBuffer hoverUBO;

    GizmoType selectedGizmoType = GizmoType::TRANSLATE;
    Gizmo* currentGizmo;

    uint32_t nextID = 0;
    std::deque<uint32_t> freeIDs;
    std::unordered_set<int32_t> hoveredIDs;

    std::unique_ptr<Physics::PhysicsSystem> physicsSystem;
};

template<typename T>
void Scene::addObject(T* obj) {
    if constexpr (std::is_base_of_v<IDrawable, T>) {
        drawableObjects.push_back(static_cast<IDrawable*>(obj));
    }
    if constexpr (std::is_base_of_v<IPickable, T>) {
        pickableObjects.push_back(static_cast<IPickable*>(obj));
    }
    if constexpr (std::is_same_v<SceneObject, T>) {
        auto *body = static_cast<SceneObject*>(obj)->physicsBody;
        if (body) {
            physicsSystem->addBody(body);
        }
    }
}