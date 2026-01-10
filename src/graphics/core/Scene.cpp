#include "Scene.h"
#include <iostream>
#include <algorithm>

#include "math/MathUtils.h"
#include <graphics/core/ResourceManager.h>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/core/SceneObject.h>

#include "ui/OpenGLWindow.h"

struct BatchKey {
    Mesh* mesh;
    Shader* shader;

    bool operator<(const BatchKey& other) const {
        if (mesh != other.mesh) return mesh < other.mesh;
        return shader < other.shader;
    }
};

Scene::Scene(QOpenGLFunctions_4_5_Core* glFuncs) : funcs(glFuncs), camera(Camera(glm::vec3(0.0f, 10.0f, 30.0f))), basicShader(nullptr), cameraUBO(2*sizeof(glm::mat4), 0, funcs), hoverUBO(sizeof(glm::ivec4) * 1024, 1, funcs), selectUBO(sizeof(glm::ivec4) * 1024, 2, funcs) {
    ResourceManager::loadPrimitives();
    basicShader = ResourceManager::loadShader("assets/shaders/primitive/primitive.vert", "assets/shaders/primitive/primitive.frag", "basic");
    ResourceManager::loadShader("assets/shaders/primitive/checkerboard.vert", "assets/shaders/primitive/checkerboard.frag", "checkerboard");
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

    camera.update();

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

    // --- instanced ---
    std::map<BatchKey, std::vector<InstanceData>> batches;

    for (auto* obj : instancedDrawables) {
        BatchKey key{ obj->getMesh(), obj->getShader() };
        batches[key].push_back(obj->getInstanceData());
    }

    for (auto& [key, instances] : batches) {
        key.shader->use();
        key.mesh->drawInstanced(instances);
    }

    // --- custom ---
    for (auto* obj : customDrawables) {
        obj->draw();
    }
}

Camera *Scene::getCamera() {
    return &camera;
}

void Scene::addDrawable(IDrawable* drawable) {
    assert(drawable != nullptr);

    if (auto instanced = dynamic_cast<IInstancedDrawable*>(drawable)) {
        instancedDrawables.push_back(instanced);
    } else if (auto custom = dynamic_cast<ICustomDrawable*>(drawable)) {
        customDrawables.push_back(custom);
    }
}

void Scene::removeDrawable(IDrawable* drawable) {
    assert(drawable != nullptr);

    if (auto instanced = dynamic_cast<IInstancedDrawable*>(drawable)) {
        instancedDrawables.erase(
            std::remove(instancedDrawables.begin(), instancedDrawables.end(), instanced),
            instancedDrawables.end()
        );
    }
    else if (auto custom = dynamic_cast<ICustomDrawable*>(drawable)) {
        customDrawables.erase(
            std::remove(customDrawables.begin(), customDrawables.end(), custom),
            customDrawables.end()
        );
    }
}
