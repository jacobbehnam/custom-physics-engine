#include "RigidBody.h"

Physics::RigidBody::RigidBody(float m, glm::vec3 pos) : mass(m), position(pos) {}

void Physics::RigidBody::applyForce(const glm::vec3 &force) {
    netForce += force;
}

void Physics::RigidBody::step(float dt) {
    velocity = velocity + (netForce/mass)*dt;
    position = position + velocity * dt + (netForce/(2 * mass)) * dt * dt;
}