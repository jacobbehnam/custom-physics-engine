#include "PhysicsSystem.h"

Physics::PhysicsSystem::PhysicsSystem(const glm::vec3 &globalAccel) : globalAcceleration(globalAccel) {}

void Physics::PhysicsSystem::addBody(RigidBody *body) {
    bodies.push_back(body);
}

void Physics::PhysicsSystem::step(float dt) {
    if (physicsEnabled) {
        for (auto body : bodies) {
            body->step(dt);
        }
    }
}