#include "RigidBody.h"

#include <iostream>

#include "PointMass.h"
#include "bounding/BoxCollider.h"

Physics::RigidBody::RigidBody(float m, ICollider* col, glm::vec3 pos, bool bodyStatic) : mass(m), position(pos), collider(col), isStatic(bodyStatic) {}

Physics::RigidBody::RigidBody(ICollider* col, glm::vec3 pos, bool bodyStatic) : collider(col), isStatic(bodyStatic), position(pos), mass(0.0f) {}

void Physics::RigidBody::applyForce(const glm::vec3 &force) {
    std::lock_guard<std::mutex> lock(stateMutex);
    netForce += force;
}

void Physics::RigidBody::setForce(const std::string &name, const glm::vec3 &force) {
    std::lock_guard<std::mutex> lock(stateMutex);
    forces[name] = force;

    glm::vec3 tempNetForce(0.0f);
    for (const auto& [name, vec] : forces) {
        tempNetForce += vec;
    }
    netForce = tempNetForce;
}

glm::vec3 Physics::RigidBody::getForce(const std::string &name, BodyLock lock) const {
    if (lock == BodyLock::LOCK) {
        std::lock_guard<std::mutex> lk(stateMutex);
        return forces.find(name)->second;
    }

    return forces.find(name)->second;
}

std::map<std::string, glm::vec3> Physics::RigidBody::getAllForces(BodyLock lock) const {
    if (lock == BodyLock::LOCK) {
        std::lock_guard<std::mutex> lk(stateMutex);
        return forces;
    }

    return forces;
}

glm::vec3 Physics::RigidBody::getPosition(BodyLock lock) const {
    if (lock == BodyLock::LOCK) {
        std::lock_guard<std::mutex> lk(stateMutex);
        return position;
    }

    return position;
}

void Physics::RigidBody::setPosition(const glm::vec3 &pos) {
    std::lock_guard<std::mutex> lock(stateMutex);
    position = pos;
}

glm::vec3 Physics::RigidBody::getVelocity(BodyLock lock) const {
    if (lock == BodyLock::LOCK) {
        std::lock_guard<std::mutex> lk(stateMutex);
        return velocity;
    }

    return velocity;
}

void Physics::RigidBody::setVelocity(const glm::vec3 &vel) {
    std::lock_guard<std::mutex> lock(stateMutex);
    velocity = vel;
}

float Physics::RigidBody::getMass(BodyLock lock) const {
    if (lock == BodyLock::LOCK) {
        std::lock_guard<std::mutex> lk(stateMutex);
        return mass;
    }

    return mass;
}

void Physics::RigidBody::setMass(float newMass) {
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        mass = newMass;
    }
    setForce("Gravity", mass * glm::vec3(0.0f, -9.81f, 0.0f));
}

bool Physics::RigidBody::getIsStatic() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return isStatic;
}

void Physics::RigidBody::setWorldTransform(const glm::mat4 &M) {
    std::lock_guard<std::mutex> lock(stateMutex);
    worldMatrix = M;
}

void Physics::RigidBody::recordFrame(float t) {
    std::lock_guard<std::mutex> lock(stateMutex);
    frames.push_back( {this, t, position, velocity});
}

const std::vector<ObjectSnapshot> &Physics::RigidBody::getAllFrames() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return frames;
}

void Physics::RigidBody::clearAllFrames() {
    std::lock_guard<std::mutex> lock(stateMutex);
    frames.clear();
}

void Physics::RigidBody::loadFrame(const ObjectSnapshot &snapshot) {
    std::lock_guard<std::mutex> lock(stateMutex);
    position = snapshot.position;
    velocity = snapshot.velocity;
}

void Physics::RigidBody::step(float dt) {
    std::lock_guard<std::mutex> lock(stateMutex);
    glm::vec3 acceleration = netForce / mass;
    glm::vec3 posIncrement = velocity * dt + 0.5f * acceleration * dt * dt;
    position += posIncrement;
    worldMatrix = glm::translate(worldMatrix, posIncrement);
    glm::vec3 newAcceleration = netForce / mass; // if netForce changed during the step
    velocity += 0.5f * (acceleration + newAcceleration) * dt;
}

bool Physics::RigidBody::collidesWith(const IPhysicsBody &other) const {
    return other.collidesWithRigidBody(*this);
}

bool Physics::RigidBody::collidesWithPointMass(const PointMass &pm) const {
    glm::mat4 M;
    {
        std::lock_guard<std::mutex> lk(stateMutex);
        M = worldMatrix;
    }
    ICollider* worldCollider = collider->getTransformed(M);
    return worldCollider->contains(pm.getPosition(BodyLock::LOCK));
}

bool Physics::RigidBody::collidesWithRigidBody(const RigidBody &rb) const {
    return false;
}

bool Physics::RigidBody::resolveCollisionWith(IPhysicsBody &other) {
    return other.resolveCollisionWithRigidBody(*this);
}

bool Physics::RigidBody::resolveCollisionWithPointMass(PointMass &pm) {
    ICollider* worldCollider = collider->getTransformed(worldMatrix);
    ContactInfo ci = worldCollider->closestPoint(pm.getPosition(BodyLock::LOCK));
    if (ci.penetration < 0.0f) return false; // no overlap

    float vRel = glm::dot(pm.getVelocity(BodyLock::LOCK), ci.normal);
    if (vRel >= 0.0f) return false; // moving apart or resting

    float e = 0.0f; // restitution coefficient
    float j = -(1.0f + e) * vRel * pm.getMass(BodyLock::LOCK);

    pm.applyImpulse(j * ci.normal);
    glm::vec3 Fnet(0.0f);
    for (auto const& [n, f] : pm.getAllForces(BodyLock::LOCK)) Fnet += f;
    glm::vec3 Fn = -glm::dot(Fnet, ci.normal) * ci.normal;
    pm.setForce("Normal", Fn);

    pm.setPosition(pm.getPosition(BodyLock::LOCK) + ci.normal * ci.penetration);

    return true;
}

bool Physics::RigidBody::resolveCollisionWithRigidBody(RigidBody &rb) {
    return false;
}
