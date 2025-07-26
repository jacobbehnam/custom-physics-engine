#include "PointMass.h"
#include <iostream>

#include "RigidBody.h"

Physics::PointMass::PointMass(float m, glm::vec3 pos) : position(pos), mass(m) {
    PointMass::setForce("Gravity", m * glm::vec3(0.0f, -9.81f, 0.0f));
    PointMass::setForce("Normal", glm::vec3(0.0f));
}

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


void Physics::PointMass::step(float dt) {
    glm::vec3 acceleration = netForce / mass;
    position += velocity * dt + 0.5f * acceleration * dt * dt;
    glm::vec3 newAcceleration = netForce / mass; // if netForce changed during the step
    velocity += 0.5f * (acceleration + newAcceleration) * dt;
    if (glm::distance(velocity, glm::vec3(0.0f)) <= 0.01f)
        std::cout << position.x << "," << position.y << "," << position.z << std::endl;
}

bool Physics::PointMass::collidesWith(const IPhysicsBody &other) const {
    return other.collidesWithPointMass(*this);
}

bool Physics::PointMass::collidesWithPointMass(const PointMass &pm) const {
    float threshold = 0.01f;
    return glm::distance(position, pm.getPosition()) <= threshold;
}

bool Physics::PointMass::collidesWithRigidBody(const RigidBody &rb) const {
    return rb.collider->contains(position);
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

    float j = (2.0f * velNorm) / (mass + pm.mass); // collision‐impulse scalar
    glm::vec3 impulse = j * normal;

    // newton's third law
    applyImpulse(impulse);
    pm.applyImpulse(-impulse);
    return true;
}

bool Physics::PointMass::resolveCollisionWithRigidBody(RigidBody &rb) {
    // 1) Get contact info from the rigid body
    ContactInfo ci = rb.collider->closestPoint(position);

    // ci.penetration > 0 means no overlap (point is outside)
    if (ci.penetration > 0.0f)
        return false;

    // 2) Compute relative velocity along normal (rigid body is static, so v_rb = 0)
    float vRel = glm::dot(velocity, ci.normal);
    if (vRel >= 0.0f)
        return false;   // moving away or resting

    // 3) Compute collision impulse scalar (J = -(1+e) vRel * m)
    //    We choose restitution e = 0 (perfectly inelastic) or >0 for bounce
    constexpr float e = 0.5f;
    float j = -(1.0f + e) * vRel * mass;

    // 4) Apply impulse to this point mass
    glm::vec3 impulse = j * ci.normal;
    applyImpulse(impulse);

    // 5) Positional correction (optional, to eliminate any penetration)
    //    Move the point out along the normal by exactly the penetration depth
    position += ci.normal * (-ci.penetration);

    return true;
}
