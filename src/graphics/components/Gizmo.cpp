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
    sceneManager->addDrawable(this);
    sceneManager->addPickable(this);

    shader = ResourceManager::loadShader("assets/shaders/gizmo/gizmo.vert", "assets/shaders/gizmo/gizmo.frag", "gizmo");

    switch (type) {
        case GizmoType::TRANSLATE:
            handleMesh = ResourceManager::getMesh("gizmo_translate");
            handles.emplace_back(new TranslateHandle(target, Axis::X));
            handles.emplace_back(new TranslateHandle(target, Axis::Y));
            handles.emplace_back(new TranslateHandle(target, Axis::Z));
            break;
        case GizmoType::ROTATE:
            handleMesh = ResourceManager::getMesh("gizmo_rotate");
            handles.emplace_back(new RotateHandle(target, Axis::X));
            handles.emplace_back(new RotateHandle(target, Axis::Y));
            handles.emplace_back(new RotateHandle(target, Axis::Z));
            break;
        case GizmoType::SCALE:
            handleMesh = ResourceManager::getMesh("gizmo_scale");
            handles.emplace_back(new ScaleHandle(target, Axis::X));
            handles.emplace_back(new ScaleHandle(target, Axis::Y));
            handles.emplace_back(new ScaleHandle(target, Axis::Z));
            break;
    }
}

Gizmo::~Gizmo() {
    ownerScene->freeObjectID(objectID);
}


void Gizmo::draw() const {
    glDisable(GL_DEPTH_TEST);
    getShader()->use();

    std::vector<InstanceData> drawData;
    for (const auto& handle : handles) {
        glm::vec3 color = handle->getAxisDir(); // base RGB per axis

        // highlight hovered or active handle
        if (handle.get() == hoveredHandle || handle.get() == activeHandle) {
            color = glm::mix(color, glm::vec3(1.0f), 0.7f); // lighten
        }

        drawData.push_back({ handle->getModelMatrix(), objectID, color });
    }

    getMesh()->drawInstanced(drawData);
    glEnable(GL_DEPTH_TEST);
}

std::optional<float> Gizmo::intersectsRay(const Math::Ray& ray) const{
    Physics::Bounding::AABB localAABB = getMesh()->getLocalAABB();

    IHandle* bestHandle = nullptr;
    float closestT = std::numeric_limits<float>::infinity();

    for (const auto& handle: handles) {
        auto worldAABB = localAABB.getTransformed(handle->getModelMatrix());
        if (auto t = worldAABB->intersectRay(ray)) {
            if (*t < closestT) {
                closestT = *t;
                bestHandle = handle.get();
            }
        }
    }

    hoveredHandle = bestHandle;

    if (bestHandle)
        return closestT;

    return std::nullopt;
}

void Gizmo::handleClick(const Math::Ray& ray, float distance) {
    if (!hoveredHandle)
        return;

    activeHandle = hoveredHandle;
    activeHandle->setDragState(ray.origin + ray.dir * distance);
    isDragging = true;
}

void Gizmo::handleRelease() {
    isDragging = false;
    activeHandle = nullptr;
}

void Gizmo::handleDrag(const Math::Ray& ray) {
    if (!isDragging || !activeHandle)
        return;

    activeHandle->onDrag(ray);
}


Shader* Gizmo::getShader() const {
    return shader;
}

void Gizmo::setHovered(bool hovered) {
    isHovered = hovered;
}

bool Gizmo::getHovered() const {
    return isHovered;
}

uint32_t Gizmo::getObjectID() const {
    return objectID;
}
