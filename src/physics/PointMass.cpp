#include "PointMass.h"
#include <algorithm>
#include <iostream>

#include "RigidBody.h"
#include "physics/utils/ThermalUtils.h"

Physics::PointMass::PointMass(uint32_t id, double m, glm::vec3 pos, bool bodyStatic) : PhysicsBody(id) {
    std::lock_guard<std::mutex> lock(stateMutex);
    setPosition(pos, BodyLock::NOLOCK);
    setMass(m, BodyLock::NOLOCK);
    setIsStatic(bodyStatic, BodyLock::NOLOCK);
    setVelocity(glm::vec3(0.0f), BodyLock::NOLOCK);
    recomputeSurfaceArea();
}

Physics::PointMass::PointMass(uint32_t id, glm::vec3 pos, bool bodyStatic) : PhysicsBody(id) {
    std::lock_guard<std::mutex> lock(stateMutex);
    setPosition(pos, BodyLock::NOLOCK);
    setIsStatic(bodyStatic, BodyLock::NOLOCK);
    setMass(1.0f, BodyLock::NOLOCK);
    recomputeSurfaceArea();
}

void Physics::PointMass::setMass(double newMass, BodyLock lock) {
    PhysicsBody::setMass(newMass, lock);
    recomputeSurfaceArea();
}

void Physics::PointMass::setThermalProperty(const ThermalProperties& newProps, BodyLock lock) {
    PhysicsBody::setThermalProperty(newProps, lock);
    recomputeSurfaceArea();
}

void Physics::PointMass::recomputeSurfaceArea() {
    double curMass = getMass(BodyLock::NOLOCK);
    float density = getThermalProperties(BodyLock::NOLOCK).density;
    if (density > 0.0f) {
        double volume = curMass / density;
        float radius = std::cbrt((3.0f * volume) / (4.0f * glm::pi<float>()));
        surfaceArea = 4.0f * glm::pi<float>() * radius * radius;
    }
}

void Physics::PointMass::applyImpulse(const glm::vec3 &impulse, BodyLock lock) {
    glm::vec3 curVel;
    double curMass;

    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    curVel = getVelocity(BodyLock::NOLOCK);
    curMass = getMass(BodyLock::NOLOCK);

    setVelocity(curVel + impulse * static_cast<float>(1.0 / curMass), BodyLock::NOLOCK);
}

void Physics::PointMass::recordFrame(float t, BodyLock lock) {
    std::unique_lock<std::mutex> maybeLock;
    if (lock == BodyLock::LOCK)
        maybeLock = std::unique_lock<std::mutex>(stateMutex);

    frames.push_back( {this, t, getPosition(BodyLock::NOLOCK), getVelocity(BodyLock::NOLOCK), static_cast<float>(getThermalProperties(BodyLock::NOLOCK).tempK)} );
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

    glm::vec3 acceleration = getNetForce(BodyLock::NOLOCK) / static_cast<float>(getMass(BodyLock::NOLOCK));
    setPosition(getPosition(BodyLock::NOLOCK) + (getVelocity(BodyLock::NOLOCK) * dt + 0.5f * acceleration * dt * dt), BodyLock::NOLOCK);
    glm::vec3 newAcceleration = getNetForce(BodyLock::NOLOCK) / static_cast<float>(getMass(BodyLock::NOLOCK)); // if netForce changed during the step
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

bool Physics::PointMass::resolveCollisionWith(float dt, PhysicsBody &other) {
    return other.resolveCollisionWithPointMass(dt, *this);
}

bool Physics::PointMass::resolveCollisionWithPointMass(float dt, PointMass &pm) {
    // elastic collision
    // compute normal and relative velocity
    std::lock_guard<std::mutex> lock(stateMutex);
    glm::vec3 normal = glm::normalize(pm.getPosition(BodyLock::NOLOCK) - getPosition(BodyLock::NOLOCK));
    glm::vec3 relVel = pm.getVelocity(BodyLock::NOLOCK) - getVelocity(BodyLock::NOLOCK);
    float velNorm = glm::dot(relVel, normal);

    if (velNorm <= 0.0f)
        return false; // moving apart, nothing applied

    float j = (2.0f * velNorm) / static_cast<float>(getMass(BodyLock::NOLOCK) + pm.getMass(BodyLock::NOLOCK)); // collision impulse scalar
    glm::vec3 impulse = j * normal;

    // newton's third law
    applyImpulse(impulse, BodyLock::NOLOCK);
    pm.applyImpulse(-impulse, BodyLock::NOLOCK);

    // Contact conduction
    ThermalProperties myProps = getThermalProperties(BodyLock::NOLOCK);
    ThermalProperties pmProps = pm.getThermalProperties(BodyLock::NOLOCK);
    const double contactArea = 0.01 * std::min(getSurfaceArea(), pm.getSurfaceArea());
    const double distance = 0.01;
    Physics::Thermal::applyConductiveExchange(
        myProps, getMass(BodyLock::NOLOCK),
        pmProps, pm.getMass(BodyLock::NOLOCK),
        contactArea, distance, dt
    );

    setThermalProperty(myProps, BodyLock::NOLOCK);
    pm.setThermalProperty(pmProps, BodyLock::NOLOCK);

    return true;
}

bool Physics::PointMass::resolveCollisionWithRigidBody(float dt, RigidBody &rb) {
    return rb.resolveCollisionWithPointMass(dt, *this);
}
