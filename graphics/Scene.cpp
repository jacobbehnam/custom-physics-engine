#include "Scene.h"

std::vector<Vertex> vertices = {
    { {-0.5f, -0.5f, -0.5f}, {0.0f,0.0f,0.0f} },
    { {0.5f, -0.5f, -0.5f}, {0.0f,0.0f,0.0f} },
    { {0.5f, 0.5f, -0.5f}, {0.0f,0.0f,0.0f} },
    { {-0.5f, 0.5f, -0.5f}, {0.0f,0.0f,0.0f} },
    { {-0.5f, -0.5f, 0.5f}, {0.0f,0.0f,0.0f} },
    { {0.5f, -0.5f, 0.5f}, {0.0f,0.0f,0.0f} },
    { {0.5f, 0.5f, 0.5f}, {0.0f,0.0f,0.0f} },
    { {-0.5f, 0.5f, 0.5f}, {0.0f,0.0f,0.0f} }
};

std::vector<unsigned int> indices {
    0, 1, 2,
    0, 3, 2,
    0, 1, 5,
    0, 4, 5,
    0, 4, 7,
    0, 3, 7,
    1, 2, 6,
    1, 5, 6,
    2, 3, 7,
    2, 6, 7,
    4, 5, 6,
    4, 7, 6
};

Scene::Scene(GLFWwindow *win) : window(win), handle(nullptr), camera(Camera(glm::vec3(0.0f, 0.0f, 3.0f))), basicShader(Shader("../vertexShader.glsl", "../fragmentShader.glsl")){
    Mesh* cubeMesh = new Mesh(vertices, indices);
    SceneObject* cube = new SceneObject(this, cubeMesh, &basicShader);
    cube->setPosition(glm::vec3(0.0f,0.0f,0.0f));
    sceneObjects.push_back(cube);

    glfwSetWindowUserPointer(window, this);
}

void Scene::draw() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjMatrix();

    // TODO: Optimize later by only setting the view and projection matrix per shader rather than per object
    for (SceneObject* obj: sceneObjects) {
        obj->getShader()->use();
        obj->getShader()->setMat4("view", view);
        obj->getShader()->setMat4("projection", projection);
        obj->draw();
    }
}

void Scene::addObject(SceneObject* obj) {
    sceneObjects.push_back(obj);
}

