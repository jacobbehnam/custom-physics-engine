#include "Gizmo.h"

#include <iostream>
#include <graphics/MathUtils.h>

#include "Scene.h"

Gizmo::Gizmo(Scene* scene, Mesh* mesh, SceneObject *tgt, Shader *shader) : target(tgt){
    scene->addObject((IDrawable*)this);
    scene->addObject((IPickable*)this);
    handles.emplace_back(new TranslateHandle(mesh, shader, target, Axis::X));
    handles.emplace_back(new TranslateHandle(mesh, shader, target, Axis::Y));
    handles.emplace_back(new TranslateHandle(mesh, shader, target, Axis::Z));
}

void Gizmo::draw() const {
    for (TranslateHandle* handle : handles) {
        handle->draw();
    }
}

bool Gizmo::rayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, float &outDistance){
    TranslateHandle* hitHandle = nullptr;
    float closestT = std::numeric_limits<float>::infinity();

    for (TranslateHandle* handle : handles) {
        const std::vector<Vertex>& verts = handle->getMesh()->getVertices();
        const std::vector<unsigned int>& indices = handle->getMesh()->getIndices();
        const glm::mat4 model = handle->getModelMatrix();

        for (int i = 0; i + 2 < indices.size(); i += 3) {
            // Converting local coordinates of the mesh to world coordinates can probably be optimized with the GPU
            const glm::vec3& v0 = glm::vec3(model * glm::vec4(verts[indices[i]].pos, 1));
            const glm::vec3& v1 = glm::vec3(model * glm::vec4(verts[indices[i+1]].pos, 1));
            const glm::vec3& v2 = glm::vec3(model * glm::vec4(verts[indices[i+2]].pos, 1));

            float outT;
            if (intersectTriangle(rayOrigin, rayDir, v0, v1, v2, outT)) {
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
    activeHandle = hitHandle;
    return (bool) hitHandle;
}

void Gizmo::handleClick(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float distance) {
    // Only fires if a handle was clicked
    activeHandle->originalPosition = target->getPosition();
    activeHandle->initialHitPoint = rayOrig + rayDir*distance;
}

void Gizmo::handleDrag(const glm::vec3 &rayOrig, const glm::vec3 &rayDir) {
    activeHandle->onDrag(rayOrig, rayDir);
}


Shader *Gizmo::getShader() const {
    // TODO: make the gizmo own the shader
    return handles[0]->getShader();
}

