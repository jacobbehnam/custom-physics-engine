#include "PointMass.h"
#include <iostream>

#include "RigidBody.h"

Physics::PointMass::PointMass(float m, glm::vec3 pos, bool bodyStatic) : position(pos), mass(m), isStatic(bodyStatic) {}

Physics::PointMass::PointMass(glm::vec3 pos, bool bodyStatic) : position(pos), isStatic(bodyStatic), mass(0.0f) {}

void Physics::PointMass::applyForce(const glm::vec3 &force) {
    netForce += force;
}

void Physics::PointMass::applyImpulse(const glm::vec3 &impulse) {
    velocity += impulse * (1.0f / mass);
}

void Physics::PointMass::setForce(const std::string &name, const glm::vec3 &force) {
    forces[name] = force;
    // Recalculate net force
    glm::vec3 tempNetForce(0.0f);
    for (const auto& [name, vec] : forces) {
        tempNetForce += vec;
    }
    netForce = tempNetForce;
}

void Physics::PointMass::setMass(float newMass) {
    mass = newMass;
    setForce("Gravity", mass * glm::vec3(0.0f, -9.81f, 0.0f));
}


void Physics::PointMass::step(float dt) {
    glm::vec3 acceleration = netForce / mass;
    position += velocity * dt + 0.5f * acceleration * dt * dt;
    glm::vec3 newAcceleration = netForce / mass; // if netForce changed during the step
    velocity += 0.5f * (acceleration + newAcceleration) * dt;
}

void Physics::PointMass::loadFrame(const ObjectSnapshot &snapshot) {
    setPosition(snapshot.position);
    setVelocity(snapshot.velocity);
}

bool Physics::PointMass::collidesWith(const IPhysicsBody &other) const {
    return other.collidesWithPointMass(*this);
}

bool Physics::PointMass::collidesWithPointMass(const PointMass &pm) const {
    float threshold = 0.01f;
    return glm::distance(position, pm.getPosition()) <= threshold;
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
    glm::vec3 normal  = glm::normalize(pm.position - position);
    glm::vec3 relVel  = pm.velocity - velocity;
    float     velNorm = glm::dot(relVel, normal);

    if (velNorm <= 0.0f)
        return false; // moving apart, nothing applied

    float j = (2.0f * velNorm) / (mass + pm.mass); // collision‐impulse scalar
    glm::vec3 impulse = j * normal;

    // newton's third law
    applyImpulse(impulse);
    pm.applyImpulse(-impulse);
    return true;
}

bool Physics::PointMass::resolveCollisionWithRigidBody(RigidBody &rb) {
    return rb.resolveCollisionWithPointMass(*this);
}
