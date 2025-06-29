#pragma once
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <graphics/Camera.h>
#include <graphics/SceneObject.h>
#include <graphics/Gizmo.h>
#include <vector>

class Scene {
public:
    Scene(GLFWwindow* win);
    ~Scene() = default;
    void draw();
    void addObject(IDrawable* obj);
    void addObject(IPickable* obj);

    void processInput(float dt);
    void handleMouseButton(int button, int action, int mods);

    void setGizmoFor(SceneObject* newTarget);
    void deleteGizmo();

    Camera* getCamera();

    // TODO: remove from public
    Gizmo* translationGizmo;
private:
    MathUtils::Ray getMouseRay();

    GLFWwindow* window;
    Camera camera;
    std::vector<IDrawable*> drawableObjects;
    std::vector<IPickable*> pickableObjects;
    Shader basicShader;

    // Mouse logic
    double mouseLastX, mouseLastY; // last FRAME x and y position
    bool mouseLeftHeld = false;
    bool mouseRightHeld = false;
    bool mouseCaptured = false;
    bool mouseDragging = false;

    double mouseLastXBeforeCapture, mouseLastYBeforeCapture; // used to restore previous mouse position after capture
};
