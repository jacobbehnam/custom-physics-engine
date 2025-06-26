#pragma once
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <graphics/Camera.h>
#include <graphics/SceneObject.h>
#include <graphics/TranslateHandle.h>
#include <vector>

class Scene {
public:
    Scene(GLFWwindow* win);
    ~Scene() = default;
    void draw();
    void addObject(SceneObject* obj);
    Camera camera;
private:
    GLFWwindow* window;
    std::vector<SceneObject*> sceneObjects;
    TranslateHandle* handle;
    Shader basicShader;
};
