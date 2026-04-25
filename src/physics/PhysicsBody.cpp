#include "physics/PhysicsBody.h"

#include <iostream>

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

float Physics::PhysicsBody::getMass(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    return mass;
}

void Physics::PhysicsBody::setMass(float newMass, BodyLock lock) {
    if (newMass <= 0.0001f) {
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

void Physics::PhysicsBody::setTemperature(float kelvin, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);
    m_temperature = std::max(0.0f, kelvin);
}

float Physics::PhysicsBody::getTemperature(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);
    return m_temperature;
}

glm::vec3 Physics::PhysicsBody::blackbodyRGB(float t) {
    if (t <= 0.0f) return glm::vec3(0.0f);
    float k = t / 100.0f;
    glm::vec3 col;

    col.r = k <= 66.0f
        ? 1.0f
        : glm::clamp(1.2929f * std::pow(k - 60.0f, -0.1332f), 0.0f, 1.0f);

    col.g = k <= 66.0f
        ? glm::clamp(0.3900f * std::log(k) - 0.6318f, 0.0f, 1.0f)
        : glm::clamp(1.1298f * std::pow(k - 60.0f, -0.0755f), 0.0f, 1.0f);

    col.b = k >= 66.0f ? 1.0f
          : k <= 19.0f ? 0.0f
          : glm::clamp(0.5432f * std::log(k - 10.0f) - 1.1963f, 0.0f, 1.0f);

    return col;
}

float Physics::PhysicsBody::temperatureToIntensity(float tempKelvin) {
    // Scaled-down Stefan-Boltzmann
    constexpr float kScale = 1.0f / (1000.0f * 1000.0f);
    return std::pow(tempKelvin, 4.0f) * kScale;
}

glm::vec3 Physics::PhysicsBody::getEmission(BodyLock lock) const {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);
    if (m_temperature <= 0.0f) return glm::vec3(0.0f);
    return blackbodyRGB(m_temperature) * temperatureToIntensity(m_temperature);
}
