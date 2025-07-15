#include "SceneManager.h"
#include "graphics/core/ResourceManager.h"

SceneManager::SceneManager(Scene *scn) : scene(scn) {
    // TODO: preload shaders in resourcemanager (rn its in Scene)
}

void SceneManager::defaultSetup() {
    Shader* basicShader = ResourceManager::getShader("basic");
    SceneObject *cube = createPrimitive(Primitive::SPHERE, basicShader, true, glm::vec3(0.0f,1.0f,0.0f));
    cube->physicsBody->applyForce(glm::vec3(-1.0f, -1.0f, 0.0f));
    SceneObject *cube2 = createPrimitive(Primitive::SPHERE, basicShader, true, glm::vec3(-1.0f, 0.0f, 0.0f));
}


SceneObject* SceneManager::createPrimitive(Primitive type, Shader *shader, bool wantPhysics, const glm::vec3 &initPos) {
    SceneObject* primitive = nullptr;
    switch (type) {
        case Primitive::CUBE:
            primitive = new SceneObject(scene, ResourceManager::getMesh("prim_cube"), shader, wantPhysics, initPos);
            break;
        case Primitive::SPHERE:
            primitive = new SceneObject(scene, ResourceManager::getMesh("prim_sphere"), shader, wantPhysics, initPos);
            break;
    }
    assert(primitive != nullptr);
    sceneObjects.append(primitive);
    scene->addObject(primitive);
    emit objectAdded(primitive);

    return primitive;
}

void SceneManager::deleteObject(SceneObject *obj) {
    if (!obj) return;

    sceneObjects.erase(
        std::remove(sceneObjects.begin(), sceneObjects.end(), obj),
        sceneObjects.end());

    scene->deleteSceneObject(obj);
    emit objectRemoved(obj);
}
