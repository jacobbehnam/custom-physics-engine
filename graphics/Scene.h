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

    bool mouseCaptured = false;
    bool isDragging = false;
};
