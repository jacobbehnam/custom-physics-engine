#include "Scene.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>

#include "math/MathUtils.h"
#include <graphics/core/ResourceManager.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>
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

    glm::vec3 renderTargetPosition;
    const glm::vec3* renderTargetPositionPtr = nullptr;
    if (snaps && camera.hasTarget()) {
        const SceneObject* target = camera.getTarget();
        const Physics::PhysicsBody* targetBody = target ? target->getPhysicsBody() : nullptr;
        for (const auto& snapshot : *snaps) {
            if (snapshot.body == targetBody) {
                renderTargetPosition = snapshot.position;
                renderTargetPositionPtr = &renderTargetPosition;
                break;
            }
        }
    }

    camera.update(renderTargetPositionPtr);
    SceneObject::setRenderOrigin(camera.position);

    float nearestSurface = std::numeric_limits<float>::max();
    float farthestSurface = 300000.0f;
    for (auto* drawable : instancedDrawables) {
        auto* obj = dynamic_cast<SceneObject*>(drawable);
        if (!obj) continue;

        const float radius = glm::compMax(glm::abs(obj->getScale())) * 0.5f;
        const float distance = glm::length(obj->getPosition() - camera.position);
        if (!std::isfinite(distance) || !std::isfinite(radius)) continue;

        nearestSurface = std::min(nearestSurface, std::max(distance - radius, 0.01f));
        farthestSurface = std::max(farthestSurface, distance + radius);
    }

    if (nearestSurface != std::numeric_limits<float>::max()) {
        camera.setClipRange(std::max(nearestSurface * 0.5f, 0.01f), farthestSurface * 1.25f);
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    cameraUBO.updateData(glm::value_ptr(camera.getProjMatrix()), sizeof(glm::mat4), 0);
    cameraUBO.updateData(glm::value_ptr(camera.getRenderViewMatrix()), sizeof(glm::mat4), sizeof(glm::mat4));

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
    std::map<BatchKey, std::vector<Rendering::InstanceData>> batches;

    for (auto* obj : instancedDrawables) {
        BatchKey key{ obj->getMesh(), obj->getShader() };
        batches[key].push_back(obj->getInstanceData());
    }

    for (auto& [key, instances] : batches) {
        key.shader->use();
        key.mesh->drawInstanced(instances);
    }

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
    } else if (auto custom = dynamic_cast<ICustomDrawable*>(drawable)) {
        customDrawables.erase(
            std::remove(customDrawables.begin(), customDrawables.end(), custom),
            customDrawables.end()
        );
    }
}
