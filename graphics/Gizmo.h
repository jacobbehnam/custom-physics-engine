#pragma once
#include <graphics/SceneObject.h>
#include <graphics/Shader.h>
#include <graphics/Camera.h>
#include <GLFW/glfw3.h>
#include <graphics/TranslateHandle.h>

class Gizmo {
public:
    Gizmo(Mesh* mesh, SceneObject* tgt, Shader* shader);

    void draw() const;

    void handleMousePress(double x, double y, const Camera& cam, GLFWwindow* window);
    void handleMouseDrag(double x, double y, const Camera& cam, GLFWwindow* window);
    void handleMouseRelease();
private:
    SceneObject* target;
    std::vector<TranslateHandle*> handles;
    TranslateHandle* activeHandle = nullptr;
};