#include "PointMass.h"
#include <iostream>

void Physics::PointMass::applyForce(const glm::vec3 &force) {
    netForce += force;
}

void Physics::PointMass::step(float dt) {
    velocity = velocity + (netForce/mass)*dt;
    position = position + velocity * dt + (netForce/(2 * mass)) * dt * dt;
}

bool Physics::PointMass::collidesWith(const IPhysicsBody &other) const {
    return other.collidesWithPointMass(*this);
}

bool Physics::PointMass::collidesWithPointMass(const PointMass &pm) const {
    float threshold = 0.01f;
    return glm::distance(position, pm.getPosition()) <= threshold;
}

bool Physics::PointMass::collidesWithRigidBody(const RigidBody &rb) const {
    return false;
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
        return false;   // moving apart, nothing applied

    float j = (2.0f * velNorm) / (mass + pm.mass);

    // apply impulse
    velocity += (j * pm.mass) * normal;
    pm.velocity -= (j * mass) * normal;
    return true;
}

bool Physics::PointMass::resolveCollisionWithRigidBody(RigidBody &rb) {
    return false;
}
