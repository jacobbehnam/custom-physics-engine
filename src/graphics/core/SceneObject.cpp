#include "SceneObject.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/core/Scene.h>
#include <math/Ray.h>
#include "physics/PointMass.h"
#include <graphics/core/SceneManager.h>

#include "ResourceManager.h"
#include "physics/bounding/BoxCollider.h"

namespace {
struct LocalRay {
    Math::Ray ray;
    float localToWorldDistance = 1.0f;
};

std::optional<LocalRay> localRayForModel(const glm::mat4& model, const Math::Ray& ray) {
    const glm::mat3 invLinear = glm::inverse(glm::mat3(model));
    const glm::vec3 modelPosition = glm::vec3(model[3]);
    const glm::vec3 localOrigin = invLinear * (ray.origin - modelPosition);
    const glm::vec3 localDir = invLinear * ray.dir;
    const float localDirLen = glm::length(localDir);
    if (!std::isfinite(localOrigin.x) || !std::isfinite(localOrigin.y) || !std::isfinite(localOrigin.z)
        || !std::isfinite(localDirLen) || localDirLen <= std::numeric_limits<float>::epsilon()) {
        return std::nullopt;
    }

    return LocalRay{
        {localOrigin, localDir / localDirLen},
        1.0f / localDirLen
    };
}

std::optional<float> intersectLocalTriangle(
    const Math::Ray& ray,
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2
) {
    constexpr float kEpsilon = 1.0e-7f;

    const glm::vec3 edge1 = v1 - v0;
    const glm::vec3 edge2 = v2 - v0;
    const glm::vec3 pvec = glm::cross(ray.dir, edge2);
    const float det = glm::dot(edge1, pvec);
    if (std::abs(det) < kEpsilon) {
        return std::nullopt;
    }

    const float invDet = 1.0f / det;
    const glm::vec3 tvec = ray.origin - v0;
    const float u = glm::dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) {
        return std::nullopt;
    }

    const glm::vec3 qvec = glm::cross(tvec, edge1);
    const float v = glm::dot(ray.dir, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) {
        return std::nullopt;
    }

    const float t = glm::dot(edge2, qvec) * invDet;
    if (!std::isfinite(t) || t <= kEpsilon) {
        return std::nullopt;
    }

    return t;
}
} // namespace

SceneObject::SceneObject(SceneManager* sceneMgr, const std::string &nameOfMesh, Shader *sdr, const CreationOptions &options, QObject* objectParent)
    : shader(sdr), ownerScene(sceneMgr->scene), sceneManager(sceneMgr), objectID(sceneMgr->scene->allocateObjectID()), parent(objectParent), meshName(nameOfMesh) {
    shader->use();
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
            physicsBody->setVelocity(o.velocity, BodyLock::LOCK);
            //physicsBody->setUnknown("v0", true);
            sceneManager->addToPhysicsSystem(physicsBody.get());
        } else if constexpr (std::is_same_v<T, RigidBodyOptions>) {
            position = o.base.position;
            scale = o.base.scale;
            rotation = o.base.rotation;
            physicsBody = std::make_unique<Physics::RigidBody>(objectID, o.mass, o.createCollider(o.base), o.base.position, o.isStatic);
            auto rb = static_cast<Physics::RigidBody*>(physicsBody.get());

            std::span<const Vertex> meshVerts = mesh->getVertices();
            std::vector<glm::vec3> verts(meshVerts.size());
            for (size_t i = 0; i < meshVerts.size(); ++i) {
                verts[i] = meshVerts[i].pos;
            }

            std::span<const unsigned int> meshInds = mesh->getIndices();
            std::vector<unsigned int> inds(meshInds.data(), meshInds.data() + meshInds.size());

            rb->setGeometry(verts, inds);
            rb->setScale(o.base.scale);
            physicsBody->setVelocity(o.velocity, BodyLock::LOCK);
            sceneManager->addToPhysicsSystem(physicsBody.get());
        } else {
            std::cout << "Problem with CreationOptions on SceneObject construction!" << std::endl;
        }
    }, options);

    orientation = glm::quat(rotation);
    if (physicsBody) {
        physicsBody->setWorldTransform(getModelMatrix(), BodyLock::LOCK);
    }
}

SceneObject::~SceneObject() {
    ownerScene->freeObjectID(objectID);
    if (physicsBody) {
        sceneManager->removeFromPhysicsSystem(physicsBody.get());
    }
}

glm::mat4 SceneObject::getModelMatrix() const {
    return buildModelMatrix(false);
}

glm::mat4 SceneObject::getRenderModelMatrix() const {
    return buildModelMatrix(true);
}

glm::mat4 SceneObject::buildModelMatrix(bool relativeToRenderOrigin) const{
    glm::vec3 currentPosition = position;
    glm::vec3 origin(0.0f);
    bool hasMappedPhysicsPosition = false;

    {
        std::lock_guard<std::mutex> lk(posMapMutex);
        origin = renderOrigin;
        if (physicsBody) {
            auto it = posMap.find(physicsBody.get());
            if (it != posMap.end()) {
                currentPosition = it->second;
                hasMappedPhysicsPosition = true;
            }
        }
    }

    if (physicsBody) {
        if (!hasMappedPhysicsPosition) {
            currentPosition = physicsBody->getPosition(BodyLock::LOCK);
        }
    }

    if (relativeToRenderOrigin) {
        currentPosition -= origin;
    }

    glm::mat4 model(1.0f);
    model = glm::translate(model, currentPosition);
    model = model * glm::mat4_cast(orientation);
    model = glm::scale(model, scale);
    return model;
}

std::optional<float> SceneObject::intersectsAABB(const Math::Ray& ray) const {
    const auto localRay = localRayForModel(getModelMatrix(), ray);
    if (!localRay) {
        return std::nullopt;
    }

    const Physics::Bounding::AABB localAABB = getMesh()->getLocalAABB();
    if (auto t = localAABB.intersectRay(localRay->ray)) {
        return t;
    }

    return std::nullopt;
}

std::optional<float> SceneObject::intersectsMesh(const Math::Ray& ray) const {
    const std::span<const Vertex> verts = mesh->getVertices();
    const std::span<const unsigned int> indices = mesh->getIndices();
    if (indices.size() < 3 || verts.empty()) {
        return std::nullopt;
    }

    const glm::mat4 model = getModelMatrix();
    const auto localRay = localRayForModel(model, ray);
    if (!localRay) {
        return std::nullopt;
    }

    float closest = std::numeric_limits<float>::infinity();
    for (size_t i = 0; i + 2u < indices.size(); i += 3u) {
        const unsigned int ia = indices[i];
        const unsigned int ib = indices[i + 1u];
        const unsigned int ic = indices[i + 2u];
        if (ia >= verts.size() || ib >= verts.size() || ic >= verts.size()) {
            continue;
        }

        const auto t = intersectLocalTriangle(localRay->ray, verts[ia].pos, verts[ib].pos, verts[ic].pos);
        if (!t) {
            continue;
        }

        const float worldDistance = *t * localRay->localToWorldDistance;
        if (std::isfinite(worldDistance) && worldDistance > 0.0f && worldDistance < closest) {
            closest = worldDistance;
        }
    }

    if (!std::isfinite(closest)) {
        return std::nullopt;
    }
    return closest;
}

std::optional<float> SceneObject::intersectsSphere(const Math::Ray& ray) const {
    const glm::mat4 model = getModelMatrix();
    const glm::dvec3 center(model[3]);
    const glm::dvec3 rayOrigin(ray.origin);
    const glm::dvec3 rayDir = glm::normalize(glm::dvec3(ray.dir));
    const double radius = 0.5 * static_cast<double>(std::max({
        glm::length(glm::vec3(model[0])),
        glm::length(glm::vec3(model[1])),
        glm::length(glm::vec3(model[2]))
    }));
    if (!std::isfinite(radius) || radius <= 0.0) {
        return std::nullopt;
    }

    const glm::dvec3 offset = rayOrigin - center;
    const double b = glm::dot(offset, rayDir);
    const double c = glm::dot(offset, offset) - radius * radius;
    const double discriminant = b * b - c;
    if (!std::isfinite(discriminant) || discriminant < 0.0f) {
        return std::nullopt;
    }

    const double sqrtDiscriminant = std::sqrt(discriminant);
    const double tNear = -b - sqrtDiscriminant;
    const double tFar = -b + sqrtDiscriminant;
    const double t = tNear > 0.0 ? tNear : tFar;
    if (!std::isfinite(t) || t <= 0.0 || t > static_cast<double>(std::numeric_limits<float>::max())) {
        return std::nullopt;
    }

    return static_cast<float>(t);
}

std::optional<float> SceneObject::intersectsRay(const Math::Ray& ray) const {
    if (meshName == "prim_sphere") {
        return intersectsSphere(ray);
    }

    auto tAABB = intersectsAABB(ray);
    if (!tAABB)
        return std::nullopt;

    auto tTri = intersectsMesh(ray);
    if (tTri) {
        return *tTri;
    }

    return std::nullopt;
}

void SceneObject::handleClick(const Math::Ray& ray, float distance) {
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
    if (physicsBody) {
        physicsBody->setWorldTransform(getModelMatrix(), BodyLock::LOCK);
        auto rb = dynamic_cast<Physics::RigidBody*>(physicsBody.get());
        if (rb) {
            rb->setScale(scl);
        }
    }
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

bool SceneObject::getHovered() const {
    return isHovered;
}

uint32_t SceneObject::getObjectID() const {
    return objectID;
}

Rendering::InstanceData SceneObject::getInstanceData() const {
    return {getRenderModelMatrix(), getObjectID(), glm::vec3(1.0f, 1.0f, 0.0f)};
}
