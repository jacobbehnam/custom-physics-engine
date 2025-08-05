#include "PointMass.h"
#include <iostream>

#include "RigidBody.h"

Physics::PointMass::PointMass(float m, glm::vec3 pos, bool bodyStatic) : position(pos), mass(m), isStatic(bodyStatic) { velocity = glm::vec3(1.0f); }

Physics::PointMass::PointMass(glm::vec3 pos, bool bodyStatic) : position(pos), isStatic(bodyStatic), mass(0.0f) {}

void Physics::PointMass::applyForce(const glm::vec3 &force) {
    std::lock_guard<std::mutex> lock(stateMutex);
    netForce += force;
}

void Physics::PointMass::setForce(const std::string &name, const glm::vec3 &force) {
    std::lock_guard<std::mutex> lock(stateMutex);
    forces[name] = force;
    // Recalculate net force
    glm::vec3 tempNetForce(0.0f);
    for (const auto& [name, vec] : forces) {
        tempNetForce += vec;
    }
    netForce = tempNetForce;
}

void Physics::PointMass::applyImpulse(const glm::vec3 &impulse) {
    std::lock_guard<std::mutex> lock(stateMutex);
    velocity += impulse * (1.0f / mass);
}

glm::vec3 Physics::PointMass::getForce(const std::string &name, BodyLock lock) const {
    if (lock == BodyLock::NOLOCK) {
        std::lock_guard<std::mutex> lk(stateMutex);
        return forces.find(name)->second;
    }

    return forces.find(name)->second;
}

std::map<std::string, glm::vec3> Physics::PointMass::getAllForces(BodyLock lock) const {
    if (lock == BodyLock::LOCK) {
        std::lock_guard<std::mutex> lk(stateMutex);
        return forces;
    }

    return forces;
}

glm::vec3 Physics::PointMass::getPosition(BodyLock lock) const {
    if (lock == BodyLock::LOCK) {
        std::lock_guard<std::mutex> lk(stateMutex);
        return position;
    }

    return position;
}

void Physics::PointMass::setPosition(const glm::vec3 &pos) {
    std::lock_guard<std::mutex> lock(stateMutex);
    position = pos;
}

glm::vec3 Physics::PointMass::getVelocity(BodyLock lock) const {
    if (lock == BodyLock::LOCK) {
        std::lock_guard<std::mutex> lk(stateMutex);
        return velocity;
    }

    return velocity;
}

void Physics::PointMass::setVelocity(const glm::vec3 &vel) {
    std::lock_guard<std::mutex> lock(stateMutex);
    velocity = vel;
}

float Physics::PointMass::getMass(BodyLock lock) const {
    if (lock == BodyLock::LOCK) {
        std::lock_guard<std::mutex> lk(stateMutex);
        return mass;
    }

    return mass;
}

void Physics::PointMass::setMass(float newMass) {
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        mass = newMass;
    }
    setForce("Gravity", mass * glm::vec3(0.0f, -9.81f, 0.0f));
}

bool Physics::PointMass::getIsStatic() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return isStatic;
}

void Physics::PointMass::setWorldTransform(const glm::mat4 &M) {
    std::lock_guard<std::mutex> lock(stateMutex);
    worldMatrix = M;
}

void Physics::PointMass::recordFrame(float t) {
    std::lock_guard<std::mutex> lock(stateMutex);
    frames.push_back( {this, t, position, velocity});
}

const std::vector<ObjectSnapshot> &Physics::PointMass::getAllFrames() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return frames;
}

void Physics::PointMass::clearAllFrames() {
    std::lock_guard<std::mutex> lock(stateMutex);
    frames.clear();
}

void Physics::PointMass::loadFrame(const ObjectSnapshot &snapshot) {
    std::lock_guard<std::mutex> lock(stateMutex);
    position = snapshot.position;
    velocity = snapshot.velocity;
}

void Physics::PointMass::step(float dt) {
    std::lock_guard<std::mutex> lock(stateMutex);
    glm::vec3 acceleration = netForce / mass;
    position += velocity * dt + 0.5f * acceleration * dt * dt;
    glm::vec3 newAcceleration = netForce / mass; // if netForce changed during the step
    velocity += 0.5f * (acceleration + newAcceleration) * dt;
}

bool Physics::PointMass::collidesWith(const IPhysicsBody &other) const {
    return other.collidesWithPointMass(*this);
}

bool Physics::PointMass::collidesWithPointMass(const PointMass &pm) const {
    float threshold = 0.01f;
    return glm::distance(getPosition(BodyLock::LOCK), pm.getPosition(BodyLock::LOCK)) <= threshold;
}

bool Physics::PointMass::collidesWithRigidBody(const RigidBody &rb) const {
    return rb.collidesWithPointMass(*this);
}

bool Physics::PointMass::resolveCollisionWith(IPhysicsBody &other) {
    return other.resolveCollisionWithPointMass(*this);
}

bool Physics::PointMass::resolveCollisionWithPointMass(PointMass &pm) {
    // elastic collision
    // compute normal and relative velocity
    glm::vec3 normal = glm::normalize(pm.getPosition(BodyLock::LOCK) - getPosition(BodyLock::LOCK));
    glm::vec3 relVel = pm.getVelocity(BodyLock::LOCK) - getVelocity(BodyLock::LOCK);
    float velNorm = glm::dot(relVel, normal);

    if (velNorm <= 0.0f)
        return false; // moving apart, nothing applied

    float j = (2.0f * velNorm) / (getMass(BodyLock::LOCK) + pm.getMass(BodyLock::LOCK)); // collision‐impulse scalar
    glm::vec3 impulse = j * normal;

    // newton's third law
    applyImpulse(impulse);
    pm.applyImpulse(-impulse);
    return true;
}

bool Physics::PointMass::resolveCollisionWithRigidBody(RigidBody &rb) {
    return rb.resolveCollisionWithPointMass(*this);
}
