#include "PhysicsSystem.h"
#include <iostream>

#include "PointMass.h"

namespace Physics {
    class PointMass;
}

Physics::PhysicsSystem::PhysicsSystem(const glm::vec3 &globalAccel) : globalAcceleration(globalAccel) {}

void Physics::PhysicsSystem::addBody(IPhysicsBody *body) {
    bodies.push_back(body);
}

void Physics::PhysicsSystem::step(float dt) {
    if (!physicsEnabled) return;

    for (auto body : bodies) {
        body->step(dt);
    }

    for (int i = 0; i < bodies.size(); ++i) {
        for (int j = i + 1; j < bodies.size(); ++j) {
            PointMass* a = dynamic_cast<PointMass*>(bodies[i]);
            PointMass* b = dynamic_cast<PointMass*>(bodies[j]);

            // if (a->isStatic() && b->isStatic()) continue;

            if (a->collidesWith(*b)) {
                std::cout << "hi" << std::endl;
            }
        }
    }
}