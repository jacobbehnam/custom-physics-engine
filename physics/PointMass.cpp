#include "PointMass.h"
#include <iostream>

#include "RigidBody.h"

Physics::PointMass::PointMass(float m, glm::vec3 pos, bool bodyStatic) {
    std::lock_guard<std::mutex> lock(stateMutex);
    setPosition(pos, BodyLock::NOLOCK);
    setMass(m, BodyLock::NOLOCK);
    setIsStatic(bodyStatic, BodyLock::NOLOCK);
    setVelocity(glm::vec3(1.0f), BodyLock::NOLOCK);
}

Physics::PointMass::PointMass(glm::vec3 pos, bool bodyStatic) {
    std::lock_guard<std::mutex> lock(stateMutex);
    setPosition(pos, BodyLock::NOLOCK);
    setIsStatic(bodyStatic, BodyLock::NOLOCK);
    setMass(1.0f, BodyLock::NOLOCK);
}

void Physics::PointMass::applyImpulse(const glm::vec3 &impulse, BodyLock lock) {
    glm::vec3 curVel;
    float curMass;

    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    curVel = getVelocity(BodyLock::NOLOCK);
    curMass = getMass(BodyLock::NOLOCK);

    setVelocity(curVel + impulse * (1.0f / curMass), BodyLock::NOLOCK);
}

void Physics::PointMass::recordFrame(float t, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    frames.push_back( {this, t, getPosition(BodyLock::NOLOCK), getVelocity(BodyLock::NOLOCK)} );
}

void Physics::PointMass::loadFrame(const ObjectSnapshot &snapshot, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    setPosition(snapshot.position, BodyLock::NOLOCK);
    setVelocity(snapshot.velocity, BodyLock::NOLOCK);
}

void Physics::PointMass::step(float dt, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    glm::vec3 acceleration = getNetForce(BodyLock::NOLOCK) / getMass(BodyLock::NOLOCK);
    setPosition(getPosition(BodyLock::NOLOCK) + (getVelocity(BodyLock::NOLOCK) * dt + 0.5f * acceleration * dt * dt), BodyLock::NOLOCK);
    glm::vec3 newAcceleration = getNetForce(BodyLock::NOLOCK) / getMass(BodyLock::NOLOCK); // if netForce changed during the step
    setVelocity(getVelocity(BodyLock::NOLOCK) + 0.5f * (acceleration + newAcceleration) * dt, BodyLock::NOLOCK);
}

bool Physics::PointMass::collidesWith(const PhysicsBody &other) const {
    return other.collidesWithPointMass(*this);
}

bool Physics::PointMass::collidesWithPointMass(const PointMass &pm) const {
    float threshold = 0.01f;
    return glm::distance(getPosition(BodyLock::LOCK), pm.getPosition(BodyLock::LOCK)) <= threshold;
}

bool Physics::PointMass::collidesWithRigidBody(const RigidBody &rb) const {
    return rb.collidesWithPointMass(*this);
}

bool Physics::PointMass::resolveCollisionWith(PhysicsBody &other) {
    return other.resolveCollisionWithPointMass(*this);
}

bool Physics::PointMass::resolveCollisionWithPointMass(PointMass &pm) {
    // elastic collision
    // compute normal and relative velocity
    std::lock_guard<std::mutex> lock(stateMutex);
    glm::vec3 normal = glm::normalize(pm.getPosition(BodyLock::NOLOCK) - getPosition(BodyLock::NOLOCK));
    glm::vec3 relVel = pm.getVelocity(BodyLock::NOLOCK) - getVelocity(BodyLock::NOLOCK);
    float velNorm = glm::dot(relVel, normal);

    if (velNorm <= 0.0f)
        return false; // moving apart, nothing applied

    float j = (2.0f * velNorm) / (getMass(BodyLock::NOLOCK) + pm.getMass(BodyLock::NOLOCK)); // collision‐impulse scalar
    glm::vec3 impulse = j * normal;

    // newton's third law
    applyImpulse(impulse, BodyLock::NOLOCK);
    pm.applyImpulse(-impulse, BodyLock::NOLOCK);
    return true;
}

bool Physics::PointMass::resolveCollisionWithRigidBody(RigidBody &rb) {
    return rb.resolveCollisionWithPointMass(*this);
}
