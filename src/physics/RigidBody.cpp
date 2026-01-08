#include "RigidBody.h"

#include <iostream>

#include "PointMass.h"
#include "bounding/BoxCollider.h"

Physics::RigidBody::RigidBody(uint32_t id, float m, std::unique_ptr<Bounding::ICollider> col, glm::vec3 pos, bool bodyStatic) : PhysicsBody(id) {
    std::lock_guard<std::mutex> lock(stateMutex);
    setMass(m, BodyLock::NOLOCK);
    setPosition(pos, BodyLock::NOLOCK);
    collider = std::move(col);
    setIsStatic(bodyStatic, BodyLock::NOLOCK);
}

Physics::RigidBody::RigidBody(uint32_t id, std::unique_ptr<Bounding::ICollider> col, glm::vec3 pos, bool bodyStatic) : PhysicsBody(id) {
    std::lock_guard<std::mutex> lock(stateMutex);
    collider = std::move(col);
    setIsStatic(bodyStatic, BodyLock::NOLOCK);
    setPosition(pos, BodyLock::NOLOCK);
    setMass(1.0f, BodyLock::NOLOCK);
}

void Physics::RigidBody::recordFrame(float t, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    frames.push_back( {this, t, getPosition(BodyLock::NOLOCK), getVelocity(BodyLock::NOLOCK)} );
}

void Physics::RigidBody::loadFrame(const ObjectSnapshot &snapshot, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    setPosition(snapshot.position, BodyLock::NOLOCK);
    setVelocity(snapshot.velocity, BodyLock::NOLOCK);
}

void Physics::RigidBody::step(float dt, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    glm::vec3 acceleration = getNetForce(BodyLock::NOLOCK) / getMass(BodyLock::NOLOCK);
    glm::vec3 posIncrement = getVelocity(BodyLock::NOLOCK) * dt + 0.5f * acceleration * dt * dt;
    setPosition(getPosition(BodyLock::NOLOCK) + posIncrement, BodyLock::NOLOCK);
    setWorldTransform(glm::translate(getWorldTransform(BodyLock::NOLOCK), posIncrement), BodyLock::NOLOCK);
    glm::vec3 newAcceleration = getNetForce(BodyLock::NOLOCK) / getMass(BodyLock::NOLOCK); // if netForce changed during the step
    setVelocity(getVelocity(BodyLock::NOLOCK) + 0.5f * (acceleration + newAcceleration) * dt, BodyLock::NOLOCK);
}

bool Physics::RigidBody::collidesWith(const PhysicsBody &other) const {
    return other.collidesWithRigidBody(*this);
}

bool Physics::RigidBody::collidesWithPointMass(const PointMass &pm) const {
    glm::mat4 M = getWorldTransform(BodyLock::LOCK);
    auto worldCollider = collider->getTransformed(M);
    return worldCollider->contains(pm.getPosition(BodyLock::LOCK));
}

bool Physics::RigidBody::collidesWithRigidBody(const RigidBody &rb) const {
    return false;
}

bool Physics::RigidBody::resolveCollisionWith(PhysicsBody &other) {
    return other.resolveCollisionWithRigidBody(*this);
}

bool Physics::RigidBody::resolveCollisionWithPointMass(PointMass &pm) {
    std::lock_guard<std::mutex> lock(stateMutex);
    auto worldCollider = collider->getTransformed(getWorldTransform(BodyLock::NOLOCK));
    Bounding::ContactInfo ci = worldCollider->closestPoint(pm.getPosition(BodyLock::NOLOCK));
    if (ci.penetration < 0.0f) return false; // no overlap

    float vRel = glm::dot(pm.getVelocity(BodyLock::NOLOCK), ci.normal);
    if (vRel >= 0.0f) return false; // moving apart or resting

    float e = 0.0f; // restitution coefficient
    float j = -(1.0f + e) * vRel * pm.getMass(BodyLock::NOLOCK);

    pm.applyImpulse(j * ci.normal, BodyLock::NOLOCK);
    glm::vec3 Fnet(0.0f);
    for (auto const& [n, f] : pm.getAllForces(BodyLock::NOLOCK)) Fnet += f;
    glm::vec3 Fn = -glm::dot(Fnet, ci.normal) * ci.normal;

    pm.setForce("Normal", Fn, BodyLock::NOLOCK);
    pm.setPosition(pm.getPosition(BodyLock::LOCK) + ci.normal * ci.penetration, BodyLock::NOLOCK);

    return true;
}

bool Physics::RigidBody::resolveCollisionWithRigidBody(RigidBody &rb) {
    return false;
}
