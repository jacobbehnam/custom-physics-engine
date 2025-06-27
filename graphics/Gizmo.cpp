#include "Gizmo.h"

Gizmo::Gizmo(Mesh* mesh, SceneObject *tgt, Shader *shader) : target(tgt){
    handles.emplace_back(new TranslateHandle(mesh, shader, target, Axis::X));
    handles.emplace_back(new TranslateHandle(mesh, shader, target, Axis::Y));
    handles.emplace_back(new TranslateHandle(mesh, shader, target, Axis::Z));
}

void Gizmo::draw() const {
    for (auto handle : handles) {
        handle->draw();
    }
}
