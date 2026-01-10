#include "Gizmo.h"

#include <iostream>
#include <math/MathUtils.h>
#include <graphics/components/TranslateHandle.h>
#include <graphics/components/RotateHandle.h>

#include "ScaleHandle.h"
#include "graphics/core/ResourceManager.h"
#include "graphics/core/SceneManager.h"
#include "graphics/core/SceneObject.h"

Gizmo::Gizmo(GizmoType type, SceneManager* sceneManager, SceneObject *tgt) : target(tgt), ownerScene(sceneManager->scene), objectID(sceneManager->scene->allocateObjectID()){
    Scene* scene = sceneManager->scene;
    sceneManager->addDrawable(this);
    sceneManager->addPickable(this);

    shader = ResourceManager::loadShader("assets/shaders/gizmo/gizmo.vert", "assets/shaders/gizmo/gizmo.frag", "gizmo");
    auto allocID = [&]() { return scene->allocateObjectID(); };

    switch (type) {
        case GizmoType::TRANSLATE:
            handleMesh = ResourceManager::getMesh("gizmo_translate");
            handles.emplace_back(new TranslateHandle(target, Axis::X, allocID()));
            handles.emplace_back(new TranslateHandle(target, Axis::Y, allocID()));
            handles.emplace_back(new TranslateHandle(target, Axis::Z, allocID()));
            break;
        case GizmoType::ROTATE:
            handleMesh = ResourceManager::getMesh("gizmo_rotate");
            handles.emplace_back(new RotateHandle(target, Axis::X, allocID()));
            handles.emplace_back(new RotateHandle(target, Axis::Y, allocID()));
            handles.emplace_back(new RotateHandle(target, Axis::Z, allocID()));
            break;
        case GizmoType::SCALE:
            handleMesh = ResourceManager::getMesh("gizmo_scale");
            handles.emplace_back(new ScaleHandle(target, Axis::X, allocID()));
            handles.emplace_back(new ScaleHandle(target, Axis::Y, allocID()));
            handles.emplace_back(new ScaleHandle(target, Axis::Z, allocID()));
            break;
    }
}

Gizmo::~Gizmo() {
    for (auto handle : handles) {
        ownerScene->freeObjectID(handle->getObjectID());
        delete handle;
    }
    ownerScene->freeObjectID(objectID);
}


void Gizmo::draw() const {
    glDisable(GL_DEPTH_TEST);
    getShader()->use();
    std::vector<InstanceData> drawData;
    for (IHandle* handle : handles) {
        uint32_t passedObjectID;
        if (activeHandle && isDragging)
            passedObjectID = handle->getObjectID();
        else
            passedObjectID = objectID;
        InstanceData handleData = {handle->getModelMatrix(), passedObjectID, handle->getAxisDir()};
        drawData.push_back(handleData);
    }
    getMesh()->drawInstanced(drawData);
    glEnable(GL_DEPTH_TEST);
}

bool Gizmo::rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance){
    Physics::Bounding::AABB localAABB = getMesh()->getLocalAABB();
    IHandle* hitHandle = nullptr;
    float closestT = std::numeric_limits<float>::infinity();

    for (IHandle* handle : handles) {
        auto worldAABB = localAABB.getTransformed(handle->getModelMatrix());
        if (auto outT = worldAABB->intersectRay(Math::Ray{rayOrigin, rayDir})) {
            if (*outT < closestT) {
                closestT = *outT;
                hitHandle = handle;
            }
        }
    }

    if (hitHandle) {
        outDistance = closestT;
        if (!activeHandle || !isDragging) {
            activeHandle = hitHandle;
        }
    }

    return hitHandle != nullptr;
}

void Gizmo::handleClick(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float distance) {
    // Only fires if a handle was clicked
    activeHandle->setDragState(rayOrig + rayDir*distance);
    isDragging = true;
}

void Gizmo::handleRelease() {
    isDragging = false;
    activeHandle = nullptr;
}


void Gizmo::handleDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir) {
    activeHandle->onDrag(rayOrig, rayDir);
}


Shader* Gizmo::getShader() const {
    return shader;
}

void Gizmo::setHovered(bool hovered) {
    isHovered = hovered;
}

bool Gizmo::getHovered() {
    return isHovered;
}

uint32_t Gizmo::getObjectID() const {
    return objectID;
}
