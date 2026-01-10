#include "SceneObject.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/core/Scene.h>
#include <math/Ray.h>
#include "physics/PointMass.h"
#include <graphics/core/SceneManager.h>
#include <graphics/components/ComputeShader.h>
#include <QOpenGLVersionFunctionsFactory>
#include <QOpenGLFunctions_4_5_Core>

#include "ResourceManager.h"
#include "physics/bounding/BoxCollider.h"

SceneObject::SceneObject(SceneManager* sceneMgr, const std::string &nameOfMesh, Shader *sdr, const CreationOptions &options, QObject* objectParent)
    : shader(sdr), ownerScene(sceneMgr->scene), sceneManager(sceneMgr), objectID(sceneMgr->scene->allocateObjectID()), parent(objectParent) {
    shader->use();
    shader->setVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));
    shader->setBool("isHovered", false);

    assert((mesh = ResourceManager::getMesh(nameOfMesh)));

    creationOptions = options;
    std::visit([&](auto&& o) {
        using T = std::decay_t<decltype(o)>;

        if constexpr (std::is_same_v<T, ObjectOptions>) {
            position = o.position;
            scale = o.scale;
            rotation = o.rotation;
        } else if constexpr (std::is_same_v<T, PointMassOptions>) {
            position = o.base.position;
            scale = o.base.scale;
            rotation = o.base.rotation;
            physicsBody = std::make_unique<Physics::PointMass>(objectID, o.mass, o.base.position, o.isStatic);
            //physicsBody->setUnknown("v0", true);
            sceneManager->addToPhysicsSystem(physicsBody.get());
        } else if constexpr (std::is_same_v<T, RigidBodyOptions>) {
            position = o.base.position;
            scale = o.base.scale;
            rotation = o.base.rotation;
            physicsBody = std::make_unique<Physics::RigidBody>(objectID, o.mass, o.createCollider(o.base), o.base.position, o.isStatic);
            sceneManager->addToPhysicsSystem(physicsBody.get());
        } else {
            std::cout << "Problem with CreationOptions on SceneObject construction!" << std::endl;
        }
    }, options);
}

SceneObject::~SceneObject() {
    ownerScene->freeObjectID(objectID);
    if (physicsBody) {
        sceneManager->removeFromPhysicsSystem(physicsBody.get());
    }
}

glm::mat4 SceneObject::getModelMatrix() const{
    glm::vec3 currentPosition = position;
    if (physicsBody) {
        std::lock_guard<std::mutex> lk(posMapMutex);
        auto it = posMap.find(physicsBody.get());
        if (it != posMap.end())
            currentPosition = it->second;
    }
    glm::mat4 model(1.0f);
    model = glm::translate(model, currentPosition);
    model = model * glm::mat4_cast(orientation);
    model = glm::scale(model, scale);
    return model;
}

bool SceneObject::intersectsAABB(const glm::vec3 &orig, const glm::vec3 &dir, float &outT) const {
    Physics::Bounding::AABB localAABB = getMesh()->getLocalAABB();
    auto worldAABB = localAABB.getTransformed(getModelMatrix());

    if (auto t = worldAABB->intersectRay(Math::Ray{orig, dir})) {
        outT = *t;
        return true;
    }

    return false;
}

bool SceneObject::intersectsMesh(const glm::vec3 &orig, const glm::vec3 &dir, float &outT) const {
    const std::vector<Vertex>& verts = mesh->getVertices();
    std::vector<glm::vec3> vertPositions;
    vertPositions.reserve(verts.size());
    for (auto vert : verts) {
        vertPositions.push_back({vert.pos});
    }
    const size_t triCount = verts.size() / 3;

    const std::vector<unsigned int>& indices = mesh->getIndices();
    auto *glFuncs = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_5_Core>(QOpenGLContext::currentContext());
    ComputeShader compute("assets/shaders/meshIntersection.comp", glFuncs);
    (void) compute.createSSBO(vertPositions.data(), vertPositions.size() * sizeof(glm::vec3), 0);
    (void) compute.createSSBO(indices.data(), indices.size() * sizeof(unsigned int), 1);
    std::vector<float> initDists(triCount, -1.0f);
    unsigned int distancesSSBO = compute.createSSBO(initDists.data(), initDists.size() * sizeof(float), 2);
    compute.use();
    compute.setVec3("rayOrig", orig);
    compute.setVec3("rayDir", dir);
    compute.setMat4("modelMatrix", getModelMatrix());

    constexpr unsigned int groupSize = 64;
    unsigned int groups = (triCount + groupSize - 1) / groupSize;
    compute.dispatch(groups);

    auto distances = compute.readSSBO<float>(distancesSSBO, triCount);
    auto minIt = std::min_element(distances.begin(), distances.end());
    if (minIt == distances.end()) {
        return false;
    }
    outT = *minIt;
    return true;
}



bool SceneObject::rayIntersection(glm::vec3 orig, glm::vec3 dir, float &outT) {
    float tAABB;
    if (!intersectsAABB(orig, dir, tAABB))
        return false;

    float tTri;
    if (intersectsMesh(orig, dir, tTri)) {
        outT = tTri;
        return true;
    }

    return false;
}

void SceneObject::handleClick(const glm::vec3 &rayOrig, const glm::vec3 &rayDir, float distance) {
    sceneManager->setGizmoFor(this);
}

void SceneObject::setPosition(const glm::vec3 &pos) {
    if (physicsBody) {
        physicsBody->setPosition(pos, BodyLock::LOCK);
        {
            std::lock_guard<std::mutex> lk(posMapMutex);
            posMap[physicsBody.get()] = pos;
        }
        physicsBody->setWorldTransform(getModelMatrix(), BodyLock::LOCK);
    } else {
        position = pos;
    }
}

void SceneObject::setRotation(const glm::vec3 &euler) {
    orientation = glm::quat(euler);
    if (physicsBody)
        physicsBody->setWorldTransform(getModelMatrix(), BodyLock::LOCK);
}

void SceneObject::setScale(const glm::vec3 &scl) {
    scale = scl;
    if (physicsBody)
        physicsBody->setWorldTransform(getModelMatrix(), BodyLock::LOCK);
}

glm::vec3 SceneObject::getPosition() const{
    if (physicsBody)
        return physicsBody->getPosition(BodyLock::NOLOCK);
    return position;
}

glm::vec3 SceneObject::getRotation() const {
    return glm::eulerAngles(orientation);
}

Shader* SceneObject::getShader() const {
    return shader;
}

void SceneObject::setRotationQuat(const glm::quat &q) {
    orientation = q;
    if (physicsBody)
        physicsBody->setWorldTransform(getModelMatrix(), BodyLock::LOCK);
}
glm::quat SceneObject::getRotationQuat()   const {
    return orientation;
}

glm::vec3 SceneObject::getScale() const {
    return scale;
}

void SceneObject::setHovered(bool hovered) {
    isHovered = hovered;
}

bool SceneObject::getHovered() {
    return isHovered;
}

uint32_t SceneObject::getObjectID() const {
    return objectID;
}


