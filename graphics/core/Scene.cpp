#include "Scene.h"
#include <iostream>
#include <algorithm>

#include "graphics/utils/MathUtils.h"
#include <graphics/core/ResourceManager.h>
#include <glm/gtc/type_ptr.hpp>

#include "ui/OpenGLWindow.h"

Scene::Scene(OpenGLWindow* win) : window(win), physicsSystem(std::make_unique<Physics::PhysicsSystem>()), camera(Camera(glm::vec3(0.0f, 0.0f, 3.0f))), basicShader(nullptr), cameraUBO(2*sizeof(glm::mat4), 0), hoverUBO(sizeof(glm::ivec4) * 1024, 1), selectUBO(sizeof(glm::ivec4) * 1024, 2) {
    ResourceManager::loadPrimitives();
    basicShader = ResourceManager::loadShader("../shaders/primitive/primitive.vert", "../shaders/primitive/primitive.frag", "basic");
}


uint32_t Scene::allocateObjectID() {
    if (!freeIDs.empty()) {
        uint32_t id = freeIDs.back();
        freeIDs.pop_back();
        return id;
    }
    return nextID++;
}

void Scene::freeObjectID(uint32_t objID) {
    freeIDs.push_back(objID);
}

void Scene::draw(const std::unordered_set<uint32_t>& hoveredIDs, const std::unordered_set<uint32_t>& selectedIDs) {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    cameraUBO.updateData(glm::value_ptr(camera.getProjMatrix()), sizeof(glm::mat4), 0);
    cameraUBO.updateData(glm::value_ptr(camera.getViewMatrix()), sizeof(glm::mat4), sizeof(glm::mat4));

    std::vector<glm::ivec4> hoverVec(1024, glm::ivec4(0));
    for (uint32_t id : hoveredIDs) {
        hoverVec[id] = glm::ivec4(1);
    }

    std::vector<glm::ivec4> selectVec(1024, glm::ivec4(0));
    for (uint32_t id : selectedIDs) {
        std::cout << id << ",";
        selectVec[id] = glm::ivec4(1);
    }
    std::cout << std::endl;

    hoverUBO.updateData(hoverVec.data(), hoverVec.size() * sizeof(glm::ivec4));
    selectUBO.updateData(selectVec.data(), selectVec.size() * sizeof(glm::ivec4));

    // === INSTANCED DRAWING FOR CUBES ===
    std::vector<InstanceData> cubeInstances;
    Mesh* cubeMesh = ResourceManager::getMesh("prim_sphere");
    for (IDrawable* obj : drawableObjects) {
        if (obj->getMesh() == cubeMesh) {
            InstanceData instance;
            instance.model = obj->getModelMatrix();
            instance.objectID = obj->getObjectID();
            cubeInstances.push_back(instance);
        }
    }

    if (!cubeInstances.empty()) {
        basicShader->use();
        cubeMesh->drawInstanced(cubeInstances);
    }

    // === NORMAL DRAWING FOR OTHER OBJECTS ===
    for (IDrawable* obj : drawableObjects) {
        if (obj->getMesh() == cubeMesh)
            continue;

        obj->draw();
    }

    // if (hovered)
    //     hoveredIDs.erase(hovered->getObjectID());
}

// void Scene::update(float dt) {
//     physicsSystem->step(dt);
//     processInput(dt);
// }

MathUtils::Ray Scene::getMouseRay() {
    QPointF mousePos = window->getMousePos();
    QSize fbSize = window->getFramebufferSize();

    return {
        camera.position,
        MathUtils::screenToWorldRayDirection(
            mousePos.x(), mousePos.y(),
            fbSize.width(), fbSize.height(),
            camera.getViewMatrix(), camera.getProjMatrix())
    };
}

void Scene::setHoveredFor(SceneObject *obj, bool flag) {
    assert(obj);
    uint32_t objID = obj->getObjectID();
    if (flag) {
        if (hoveredIDs.find(objID) == hoveredIDs.end())
            hoveredIDs.insert(objID);
    } else {
        hoveredIDs.erase(objID);
    }
}

Camera *Scene::getCamera() {
    return &camera;
}

void Scene::deleteSceneObject(SceneObject *obj) {
    if (!obj) return;

    drawableObjects.erase(
        std::remove(drawableObjects.begin(), drawableObjects.end(), static_cast<IDrawable*>(obj)),
        drawableObjects.end()
    );

    if (obj->physicsBody) {
        physicsSystem->removeBody(obj->physicsBody);
    }

    delete obj;
}

void Scene::removeDrawable(IDrawable *obj) {
    drawableObjects.erase(
        std::remove(drawableObjects.begin(), drawableObjects.end(), obj),
        drawableObjects.end());
}
