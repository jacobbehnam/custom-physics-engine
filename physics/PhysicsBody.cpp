#include "physics/PhysicsBody.h"

void Physics::PhysicsBody::applyForce(const glm::vec3 &force) {
    std::lock_guard<std::mutex> lock(stateMutex);
    netForce += force;
}

void Physics::PhysicsBody::setForce(const std::string &name, const glm::vec3 &force, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    forces[name] = force;
    // Recalculate net force
    glm::vec3 tempNetForce(0.0f);
    for (const auto& [name, vec] : forces) {
        tempNetForce += vec;
    }
    netForce = tempNetForce;
}

glm::vec3 Physics::PhysicsBody::getForce(const std::string &name, BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return forces.find(name)->second;
}

std::map<std::string, glm::vec3> Physics::PhysicsBody::getAllForces(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return forces;
}

glm::vec3 Physics::PhysicsBody::getNetForce(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return netForce;
}

glm::vec3 Physics::PhysicsBody::getPosition(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return position;
}

void Physics::PhysicsBody::setPosition(const glm::vec3 &pos, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    position = pos;
}

glm::vec3 Physics::PhysicsBody::getVelocity(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return velocity;
}

void Physics::PhysicsBody::setVelocity(const glm::vec3 &vel, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    velocity = vel;
}

float Physics::PhysicsBody::getMass(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return mass;
}

void Physics::PhysicsBody::setMass(float newMass, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);
    mass = newMass;
}

bool Physics::PhysicsBody::getIsStatic(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return isStatic;
}

void Physics::PhysicsBody::setIsStatic(bool flag, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    isStatic = flag;
}

glm::mat4 Physics::PhysicsBody::getWorldTransform(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return worldMatrix;
}

void Physics::PhysicsBody::setWorldTransform(const glm::mat4 &M, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    worldMatrix = M;
}

std::vector<ObjectSnapshot> Physics::PhysicsBody::getAllFrames() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return frames;
}

void Physics::PhysicsBody::clearAllFrames() {
    std::lock_guard<std::mutex> lock(stateMutex);
    frames.clear();
}