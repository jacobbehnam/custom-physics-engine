#include "Gizmo.h"

#include <iostream>
#include <graphics/utils/MathUtils.h>
#include <graphics/components/TranslateHandle.h>
#include <graphics/components/RotateHandle.h>

#include "ScaleHandle.h"
#include "graphics/core/ResourceManager.h"
#include "graphics/core/SceneManager.h"

Gizmo::Gizmo(GizmoType type, SceneManager* sceneManager, Mesh* mesh, SceneObject *tgt) : target(tgt), ownerScene(sceneManager->scene), objectID(sceneManager->scene->allocateObjectID()){
    Scene* scene = sceneManager->scene;
    sceneManager->addDrawable(this);
    sceneManager->addPickable(this);

    shader = ResourceManager::loadShader("../shaders/gizmo/gizmo.vert", "../shaders/gizmo/gizmo.frag", "gizmo");

    switch (type) {
        case GizmoType::TRANSLATE:
            handles.emplace_back(new TranslateHandle(mesh, shader, target, Axis::X, scene->allocateObjectID()));
            handles.emplace_back(new TranslateHandle(mesh, shader, target, Axis::Y, scene->allocateObjectID()));
            handles.emplace_back(new TranslateHandle(mesh, shader, target, Axis::Z, scene->allocateObjectID()));
            break;
        case GizmoType::ROTATE:
            handles.emplace_back(new RotateHandle(mesh, shader, target, Axis::X, scene->allocateObjectID()));
            handles.emplace_back(new RotateHandle(mesh, shader, target, Axis::Y, scene->allocateObjectID()));
            handles.emplace_back(new RotateHandle(mesh, shader, target, Axis::Z, scene->allocateObjectID()));
            break;
        case GizmoType::SCALE:
            handles.emplace_back(new ScaleHandle(mesh, shader, target, Axis::X, scene->allocateObjectID()));
            handles.emplace_back(new ScaleHandle(mesh, shader, target, Axis::Y, scene->allocateObjectID()));
            handles.emplace_back(new ScaleHandle(mesh, shader, target, Axis::Z, scene->allocateObjectID()));
            break;
    }
}

Gizmo::~Gizmo() {
    for (auto handle : handles) {
        ownerScene->freeObjectID(handle->getObjectID());
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
        float outT;
        if (localAABB.getTransformed(handle->getModelMatrix())->intersectRay(rayOrigin, rayDir, outT)) {
            if (outT < closestT) {
                closestT = outT;
                hitHandle = handle;
            }
        }
        if (hitHandle)
            outDistance = closestT;
    }
    if (!activeHandle || !isDragging)
        activeHandle = hitHandle;
    return (bool) hitHandle;
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

SceneObject* Gizmo::getTarget() {
    return target;
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
