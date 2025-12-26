#include "Scene.h"
#include <iostream>
#include <algorithm>

#include "graphics/utils/MathUtils.h"
#include <graphics/core/ResourceManager.h>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/core/SceneObject.h>

#include "ui/OpenGLWindow.h"

Scene::Scene(QOpenGLFunctions_4_5_Core* glFuncs) : funcs(glFuncs), camera(Camera(glm::vec3(0.0f, 0.0f, 3.0f))), basicShader(nullptr), cameraUBO(2*sizeof(glm::mat4), 0, funcs), hoverUBO(sizeof(glm::ivec4) * 1024, 1, funcs), selectUBO(sizeof(glm::ivec4) * 1024, 2, funcs) {
    ResourceManager::loadPrimitives();
    basicShader = ResourceManager::loadShader("assets/shaders/primitive/primitive.vert", "assets/shaders/primitive/primitive.frag", "basic");
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

void Scene::draw(const std::optional<std::vector<ObjectSnapshot>>& snaps, const std::unordered_set<uint32_t>& hoveredIDs, const std::unordered_set<uint32_t>& selectedIDs) {
    if (snaps) {
        SceneObject::PosMap posMap;
        posMap.reserve(snaps->size());
        for (const auto &s : *snaps) {
            posMap.emplace(s.body, s.position);
        }
        SceneObject::setPhysicsPosMap(posMap);
    }

    std::unordered_map<Physics::PhysicsBody*, glm::vec3> tmpMap;
    if (snaps) {
        tmpMap.reserve(snaps->size());
        for (auto &s : *snaps)
            tmpMap[s.body] = s.position;
    }

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
        selectVec[id] = glm::ivec4(1);
    }

    hoverUBO.updateData(hoverVec.data(), hoverVec.size() * sizeof(glm::ivec4));
    selectUBO.updateData(selectVec.data(), selectVec.size() * sizeof(glm::ivec4));

    // === INSTANCED DRAWING FOR CUBES ===
    std::vector<InstanceData> cubeInstances;
    Mesh* cubeMesh = ResourceManager::getMesh("prim_cube");
    std::vector<InstanceData> sphereInstances;
    Mesh* sphereMesh = ResourceManager::getMesh("prim_sphere");
    for (IDrawable* obj : drawableObjects) {
        if (obj->getMesh() == cubeMesh) {
            InstanceData instance;
            instance.model = obj->getModelMatrix();
            instance.objectID = obj->getObjectID();
            cubeInstances.push_back(instance);
        } else if (obj->getMesh() == sphereMesh) {
            InstanceData instance;
            instance.model = obj->getModelMatrix();
            instance.objectID = obj->getObjectID();
            sphereInstances.push_back(instance);
        }
    }

    if (!cubeInstances.empty()) {
        basicShader->use();
        cubeMesh->drawInstanced(cubeInstances);
    }
    if (!sphereInstances.empty()) {
        basicShader->use();
        sphereMesh->drawInstanced(sphereInstances);
    }

    // === NORMAL DRAWING FOR OTHER OBJECTS ===
    for (IDrawable* obj : drawableObjects) {
        if (obj->getMesh() == cubeMesh || obj->getMesh() == sphereMesh)
            continue;

        obj->draw();
    }
}

// void Scene::update(float dt) {
//     physicsSystem->step(dt);
//     processInput(dt);
// }

Camera *Scene::getCamera() {
    return &camera;
}

void Scene::removeDrawable(IDrawable *obj) {
    drawableObjects.erase(
        std::remove(drawableObjects.begin(), drawableObjects.end(), obj),
        drawableObjects.end());
}
