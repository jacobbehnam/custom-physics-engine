#pragma once
#include <graphics/core/Camera.h>
#include <graphics/core/SceneObject.h>
#include <graphics/components/Gizmo.h>
#include <graphics/utils/MathUtils.h>
#include <graphics/core/UniformBuffer.h>
#include <vector>
#include <deque>
#include <GLFW/glfw3.h>

class Scene {
public:
    Scene(GLFWwindow* win);
    ~Scene() = default;
    void draw();
    void addObject(IDrawable* obj);
    void addObject(IPickable* obj);

    uint32_t allocateObjectID();
    void freeObjectID(uint32_t objID);

    void processInput(float dt);
    void handleMouseButton(int button, int action, int mods);

    void setGizmoFor(SceneObject* newTarget, bool redraw = false);
    void deleteGizmo();

    Camera* getCamera();
private:
    MathUtils::Ray getMouseRay();

    GLFWwindow* window;
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

    // Mouse logic
    double mouseLastX, mouseLastY; // last FRAME x and y position
    bool mouseLeftHeld = false;
    bool mouseRightHeld = false;
    bool mouseCaptured = false;
    bool mouseDragging = false;

    double mouseLastXBeforeCapture, mouseLastYBeforeCapture; // used to restore previous mouse position after capture
};
