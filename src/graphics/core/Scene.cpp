#include "Scene.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>

#include "math/MathUtils.h"
#include <graphics/core/ResourceManager.h>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <graphics/core/SceneObject.h>

#include "ui/OpenGLWindow.h"

namespace {
constexpr float kFloatingOriginThreshold = 1.0e6f;
constexpr float kNearClipDepthFraction = 0.05f;
constexpr float kFarClipPaddingMultiplier = 1.25f;
constexpr float kMinFarthestDepthOffset = 1.0f;
constexpr float kMaxNearClipFractionOfFarClip = 0.5f;

float maxAbsComponent(const glm::vec3& v) {
    return std::max({std::abs(v.x), std::abs(v.y), std::abs(v.z)});
}

std::optional<std::pair<float, float>> viewDepthSpan(const SceneObject& obj, const glm::vec3& cameraPosition, const glm::vec3& cameraFront) {
    Mesh* mesh = obj.getMesh();
    if (!mesh) return std::nullopt;

    const auto& aabb = mesh->getLocalAABB();
    const glm::vec3 min = aabb.getAABBMin();
    const glm::vec3 max = aabb.getAABBMax();
    const glm::mat4 model = obj.getModelMatrix();

    float nearDepth = std::numeric_limits<float>::max();
    float farDepth = -std::numeric_limits<float>::max();

    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                const glm::vec3 corner(
                    x ? max.x : min.x,
                    y ? max.y : min.y,
                    z ? max.z : min.z
                );
                const glm::vec3 world = glm::vec3(model * glm::vec4(corner, 1.0f));
                const float depth = glm::dot(world - cameraPosition, cameraFront);
                nearDepth = std::min(nearDepth, depth);
                farDepth = std::max(farDepth, depth);
            }
        }
    }

    if (!std::isfinite(nearDepth) || !std::isfinite(farDepth)) {
        return std::nullopt;
    }

    return std::pair{nearDepth, farDepth};
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
    ResourceManager::loadShader("assets/shaders/debug/pathtrace.vert", "assets/shaders/debug/pathtrace.frag", "pathtrace");
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

void Scene::applyPhysicsSnapshots(const std::optional<std::vector<ObjectSnapshot>>& snaps) {
    SceneObject::PosMap renderPositions;
    if (snaps) {
        renderPositions.reserve(snaps->size());
        for (const auto &s : *snaps) {
            renderPositions.emplace(s.body, s.position);
        }
        SceneObject::setPhysicsPosMap(renderPositions);
    }
}

void Scene::updateFrameUniforms(const std::unordered_set<uint32_t>& hoveredIDs, const std::unordered_set<uint32_t>& selectedIDs) {
    const bool useFloatingOrigin = maxAbsComponent(camera.position) >= kFloatingOriginThreshold;
    SceneObject::setRenderOrigin(useFloatingOrigin ? camera.position : glm::vec3(0.0f));

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
}

void Scene::draw(const std::optional<std::vector<ObjectSnapshot>>& snaps, const std::unordered_set<uint32_t>& hoveredIDs, const std::unordered_set<uint32_t>& selectedIDs) {
    applyPhysicsSnapshots(snaps);

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
    float farthestDepth = Camera::kDefaultNearClip + kMinFarthestDepthOffset;
    bool foundVisibleDepth = false;

    for (auto* drawable : instancedDrawables) {
        auto* obj = dynamic_cast<SceneObject*>(drawable);
        if (!obj) continue;

        const auto depthSpan = viewDepthSpan(*obj, camera.position, camera.front);
        if (!depthSpan) continue;
        if (depthSpan->second <= Camera::kDefaultNearClip) continue;

        foundVisibleDepth = true;
        nearestDepth = std::min(nearestDepth, std::max(depthSpan->first, Camera::kDefaultNearClip));
        farthestDepth = std::max(farthestDepth, depthSpan->second);
    }

    if (foundVisibleDepth) {
        const float farClip = std::max(farthestDepth * kFarClipPaddingMultiplier, Camera::kDefaultFarClip);
        const float nearClip = nearestDepth == std::numeric_limits<float>::max()
            ? Camera::kDefaultNearClip
            : std::clamp(nearestDepth * kNearClipDepthFraction, Camera::kDefaultNearClip, farClip * kMaxNearClipFractionOfFarClip);
        camera.setClipRange(nearClip, farClip);
    } else {
        camera.setClipRange(Camera::kDefaultNearClip, Camera::kDefaultFarClip);
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateFrameUniforms(hoveredIDs, selectedIDs);

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
        obj->draw(snaps);
    }
}

void Scene::drawCustomDrawables(const std::optional<std::vector<ObjectSnapshot>>& snaps, const std::unordered_set<uint32_t>& hoveredIDs, const std::unordered_set<uint32_t>& selectedIDs) {
    applyPhysicsSnapshots(snaps);
    updateFrameUniforms(hoveredIDs, selectedIDs);

    for (auto* obj : customDrawables) {
        obj->draw(snaps);
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
