#include "RigidBody.h"

#include <iostream>

#include "PointMass.h"
#include "bounding/BoxCollider.h"

Physics::RigidBody::RigidBody(float m, ICollider* col, glm::vec3 pos, bool bodyStatic) : mass(m), position(pos), collider(col), isStatic(bodyStatic) {}

Physics::RigidBody::RigidBody(ICollider* col, glm::vec3 pos, bool bodyStatic) : collider(col), isStatic(bodyStatic), position(pos), mass(0.0f) {}

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

void Physics::RigidBody::setMass(float newMass) {
    mass = newMass;
    setForce("Gravity", mass * glm::vec3(0.0f, -9.81f, 0.0f));
}


void Physics::RigidBody::step(float dt) {
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
    ICollider* worldCollider = collider->getTransformed(worldMatrix);
    return worldCollider->contains(pm.getPosition());
}

bool Physics::RigidBody::collidesWithRigidBody(const RigidBody &rb) const {
    return false;
}

bool Physics::RigidBody::resolveCollisionWith(IPhysicsBody &other) {
    return other.resolveCollisionWithRigidBody(*this);
}

bool Physics::RigidBody::resolveCollisionWithPointMass(PointMass &pm) {
    ICollider* worldCollider = collider->getTransformed(worldMatrix);
    ContactInfo ci = worldCollider->closestPoint(pm.getPosition());
    if (ci.penetration < 0.0f) return false; // no overlap

    float vRel = glm::dot(pm.getVelocity(), ci.normal);
    if (vRel >= 0.0f) return false; // moving apart or resting

    float e = 0.0f; // restitution coefficient
    float j = -(1.0f + e) * vRel * pm.mass;

    pm.applyImpulse(j * ci.normal);
    glm::vec3 Fnet(0.0f);
    for (auto const& [n, f] : pm.getAllForces()) Fnet += f;
    glm::vec3 Fn = -glm::dot(Fnet, ci.normal) * ci.normal;
    pm.setForce("Normal", Fn);

    pm.setPosition(pm.getPosition() + ci.normal * ci.penetration);

    return true;
}

bool Physics::RigidBody::resolveCollisionWithRigidBody(RigidBody &rb) {
    return false;
}
