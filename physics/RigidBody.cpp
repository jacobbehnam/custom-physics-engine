#include "RigidBody.h"

#include <iostream>

#include "PointMass.h"

Physics::RigidBody::RigidBody(float m, glm::vec3 pos, ICollider* col) : mass(m), position(pos), collider(col) {}

void Physics::RigidBody::applyForce(const glm::vec3 &force) {
    netForce += force;
}

void Physics::RigidBody::setForce(const std::string &name, const glm::vec3 &force) {
    forces[name] = force;

    glm::vec3 tempNetForce(0.0f);
    for (const auto& [name, vec] : forces) {
        tempNetForce += vec;
    }
    netForce = tempNetForce;
}


void Physics::RigidBody::step(float dt) {
    velocity = velocity + (netForce/mass)*dt;
    position = position + velocity * dt + (netForce/(2 * mass)) * dt * dt;
}

bool Physics::RigidBody::collidesWith(const IPhysicsBody &other) const {
    return other.collidesWithRigidBody(*this);
}

bool Physics::RigidBody::collidesWithPointMass(const PointMass &pm) const {
    return collider->contains(pm.getPosition());
}

bool Physics::RigidBody::collidesWithRigidBody(const RigidBody &rb) const {
    return false;
}

bool Physics::RigidBody::resolveCollisionWith(IPhysicsBody &other) {
    return other.resolveCollisionWithRigidBody(*this);
}

bool Physics::RigidBody::resolveCollisionWithPointMass(PointMass &pm) {
    ContactInfo ci = collider->closestPoint(pm.getPosition());
    if (ci.penetration < 0.0f) return false; // no overlap

    float vRel = glm::dot(pm.getVelocity(), ci.normal);
    if (vRel >= 0.0f) return false; // moving apart or resting

    float e = 0.0f; // restitution coefficient
    float j = -(1.0f + e) * vRel * pm.mass;

    pm.applyImpulse(j * ci.normal);
    glm::vec3 Fg = pm.getForce("Gravity");
    glm::vec3 Fn = -glm::dot(Fg, ci.normal) * ci.normal;
    pm.setForce("Normal", Fn);

    pm.setPosition(pm.getPosition() + ci.normal * (-ci.penetration));

    return true;
}

bool Physics::RigidBody::resolveCollisionWithRigidBody(RigidBody &rb) {
    return false;
}
