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
    void addObject(SceneObject* obj);

    void processInput(float dt);
    void handleMouseButton(int button, int action, int mods);
    Camera camera;
private:
    GLFWwindow* window;
    std::vector<SceneObject*> sceneObjects;
    Gizmo* translationGizmo;
    Shader basicShader;

    bool mouseCaptured = false;
};
