#include "Gizmo.h"

#include <iostream>
#include <graphics/utils/MathUtils.h>
#include <graphics/core/Scene.h>
#include <graphics/components/TranslateHandle.h>
#include <graphics/components/RotateHandle.h>

#include "ScaleHandle.h"

Gizmo::Gizmo(GizmoType type, Scene* scene, Mesh* mesh, SceneObject *tgt, Shader *shader) : target(tgt), objectID(scene->allocateObjectID()){
    scene->addObject(static_cast<IDrawable*>(this));
    scene->addObject(static_cast<IPickable*>(this));

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

void Gizmo::draw() const {
    getShader()->use();
    std::vector<InstanceData> drawData;
    for (IHandle* handle : handles) {
        InstanceData handleData = {handle->getModelMatrix(), getObjectID()};
        drawData.push_back(handleData);
    }
    getMesh()->drawInstanced(drawData);
}

bool Gizmo::rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance){
    IHandle* hitHandle = nullptr;
    float closestT = std::numeric_limits<float>::infinity();

    for (IHandle* handle : handles) {
        const std::vector<Vertex>& verts = handle->getMesh()->getVertices();
        const std::vector<unsigned int>& indices = handle->getMesh()->getIndices();
        const glm::mat4 model = handle->getModelMatrix();

        for (int i = 0; i + 2 < indices.size(); i += 3) {
            // TODO: do not do expensive matrix multiplication on the CPU, move to GPU
            const glm::vec3& v0 = glm::vec3(model * glm::vec4(verts[indices[i]].pos, 1));
            const glm::vec3& v1 = glm::vec3(model * glm::vec4(verts[indices[i+1]].pos, 1));
            const glm::vec3& v2 = glm::vec3(model * glm::vec4(verts[indices[i+2]].pos, 1));

            float outT;
            if (MathUtils::intersectTriangle(rayOrigin, rayDir, v0, v1, v2, outT)) {
                if (outT < closestT) {
                    closestT = outT;
                    hitHandle = handle;
                }
            }
        }

        if (hitHandle) {
            outDistance = closestT;
        }
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
    // TODO: make the gizmo own the shader
    return handles[0]->getShader();
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
