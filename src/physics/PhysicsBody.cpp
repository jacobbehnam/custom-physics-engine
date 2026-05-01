#include "physics/PhysicsBody.h"

#include <algorithm>
#include <iostream>
#include "physics/utils/ThermalUtils.h"

bool Physics::PhysicsBody::isUnknown(const std::string &key, BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return unknowns.find(key) != unknowns.end();
}

void Physics::PhysicsBody::setUnknown(const std::string &key, bool active, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    if (active) {
        unknowns.insert(key);
    } else {
        unknowns.erase(key);
    }
}

void Physics::PhysicsBody::setForce(const std::string &name, const glm::vec3 &force, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    // Recalculate net force
    auto it = forces.find(name);
    if (it != forces.end()) {
        // Its old value we remove it
        netForce -= it->second;
        it->second = force;
    } else {
        forces[name] = force;
    }
    // Just add new force
    netForce += force;
}

glm::vec3 Physics::PhysicsBody::getForce(const std::string &name, BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    auto it = forces.find(name);
    if (it == forces.end()) {
        return glm::vec3(0.0f); // Return zero vector if key doesn't exist
    }

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

double Physics::PhysicsBody::getMass(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return mass;
}

void Physics::PhysicsBody::setMass(double newMass, BodyLock lock) {
    if (newMass <= 0.0) {
        std::cerr << "Warning: Invalid mass " << newMass << ". Ignoring." << std::endl;
        return;
    }

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

void Physics::PhysicsBody::clearAllFrames(BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    frames.clear();
}

void Physics::PhysicsBody::setThermalProperty(const ThermalProperties &newProps, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    thermalProps = newProps;
    thermalProps.tempK = Physics::Thermal::clampTemperature(thermalProps.tempK);
    thermalProps.specificHeat = std::max(thermalProps.specificHeat, 0.0f);
    thermalProps.emissivity = std::clamp(thermalProps.emissivity, 0.0f, 1.0f);
    thermalProps.heatTransferCoeff = std::max(thermalProps.heatTransferCoeff, 0.0f);
    thermalProps.conductivity = std::max(thermalProps.conductivity, 0.0f);
    thermalProps.density = std::max(thermalProps.density, 0.0f);
}

ThermalProperties Physics::PhysicsBody::getThermalProperties(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return thermalProps;
}
