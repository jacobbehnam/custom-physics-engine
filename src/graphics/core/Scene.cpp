#include "Scene.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>

#include "math/MathUtils.h"
#include <graphics/core/ResourceManager.h>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/core/SceneObject.h>

#include "ui/OpenGLWindow.h"

namespace {
constexpr float kFloatingOriginThreshold = 1.0e6f;
constexpr float kNearClipDepthFraction = 0.05f;

float maxAbsComponent(const glm::vec3& v) {
    return std::max({std::abs(v.x), std::abs(v.y), std::abs(v.z)});
}
}

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
    SceneObject::PosMap renderPositions;
    if (snaps) {
        renderPositions.reserve(snaps->size());
        for (const auto &s : *snaps) {
            renderPositions.emplace(s.body, s.position);
        }
        SceneObject::setPhysicsPosMap(renderPositions);
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

    float nearestDepth = std::numeric_limits<float>::max();
    float farthestDepth = Camera::kDefaultNearClip + 1.0f;
    bool foundVisibleDepth = false;

    for (auto* drawable : instancedDrawables) {
        auto* obj = dynamic_cast<SceneObject*>(drawable);
        if (!obj) continue;

        glm::vec3 position = obj->getPosition();
        if (auto* body = obj->getPhysicsBody()) {
            auto it = renderPositions.find(body);
            if (it != renderPositions.end()) {
                position = it->second;
            }
        }

        const float viewDepth = glm::dot(position - camera.position, camera.front);
        const float radius = maxAbsComponent(obj->getScale()) * 0.5f;
        if (!std::isfinite(viewDepth) || !std::isfinite(radius)) continue;
        if (viewDepth + radius <= Camera::kDefaultNearClip) continue;

        foundVisibleDepth = true;
        farthestDepth = std::max(farthestDepth, viewDepth + radius);
        if (viewDepth - radius > Camera::kDefaultNearClip) {
            nearestDepth = std::min(nearestDepth, viewDepth - radius);
        }
    }

    if (foundVisibleDepth) {
        const float farClip = std::max(farthestDepth * 1.25f, Camera::kDefaultFarClip);
        const float nearClip = nearestDepth == std::numeric_limits<float>::max()
            ? Camera::kDefaultNearClip
            : std::clamp(nearestDepth * kNearClipDepthFraction, Camera::kDefaultNearClip, farClip * 0.5f);
        camera.setClipRange(nearClip, farClip);
    } else {
        camera.setClipRange(Camera::kDefaultNearClip, Camera::kDefaultFarClip);
    }

    const bool useFloatingOrigin = maxAbsComponent(camera.position) >= kFloatingOriginThreshold;
    SceneObject::setRenderOrigin(useFloatingOrigin ? camera.position : glm::vec3(0.0f));

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::mat4 viewMatrix = useFloatingOrigin ? camera.getRenderViewMatrix() : camera.getViewMatrix();
    cameraUBO.updateData(glm::value_ptr(camera.getProjMatrix()), sizeof(glm::mat4), 0);
    cameraUBO.updateData(glm::value_ptr(viewMatrix), sizeof(glm::mat4), sizeof(glm::mat4));

    std::vector<glm::ivec4> hoverVec(1024, glm::ivec4(0));
    for (uint32_t id : hoveredIDs) {
        if (id < hoverVec.size()) {
            hoverVec[id] = glm::ivec4(1);
        }
    }

    std::vector<glm::ivec4> selectVec(1024, glm::ivec4(0));
    for (uint32_t id : selectedIDs) {
        if (id < selectVec.size()) {
            selectVec[id] = glm::ivec4(1);
        }
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
        key.shader->setVec3("renderOrigin", SceneObject::getRenderOrigin());
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
