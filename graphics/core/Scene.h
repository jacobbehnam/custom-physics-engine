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

#include "ui/OpenGLWindow.h"

enum class Primitive {
    CUBE,
    SPHERE
};

class Scene {
public:
    Scene(OpenGLWindow* win, Physics::PhysicsSystem* physicsSystem);
    ~Scene() = default;
    void draw();

    SceneObject* createPrimitive(Primitive type, Shader* shader, bool wantPhysics, const glm::vec3& initPos = glm::vec3(0.0f));

    template<typename T>
    void addObject(T* obj);

    uint32_t allocateObjectID();
    void freeObjectID(uint32_t objID);

    void processInput(float dt);
    void handleMouseButton(int button, int action, int mods);

    void setGizmoFor(SceneObject* newTarget, bool redraw = false);
    void deleteGizmo();

    Camera* getCamera();
private:
    MathUtils::Ray getMouseRay();
    IPickable* findFistHit(const std::vector<IPickable*>& objects, const MathUtils::Ray& ray, float &outT, IPickable* priority = nullptr);

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

    Physics::PhysicsSystem* physicsSystem;

    // Mouse logic
    double mouseLastX, mouseLastY; // last FRAME x and y position
    bool mouseLeftHeld = false;
    bool mouseRightHeld = false;
    bool mouseCaptured = false;
    bool mouseDragging = false;

    double mouseLastXBeforeCapture, mouseLastYBeforeCapture; // used to restore previous mouse position after capture
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