#include "PhysicsSystem.h"
#include <iostream>
#include <algorithm>
#include "PointMass.h"

namespace Physics {
    class PointMass;
}

Physics::PhysicsSystem::PhysicsSystem(const glm::vec3 &globalAccel) : globalAcceleration(globalAccel) {}

void Physics::PhysicsSystem::removeBody(IPhysicsBody *body) {
    auto it = std::remove(bodies.begin(), bodies.end(), body);
    if (it != bodies.end()) {
        bodies.erase(it, bodies.end());
    } else {
        std::cerr << "[PhysicsSystem] Warning: Tried to remove a body not in the system.\n";
    }
}

void Physics::PhysicsSystem::step(float dt) {
    if (!physicsEnabled) return;

    for (auto body : bodies) {
        if (body->getIsStatic())
            continue;
        body->recordFrame(simTime);
        body->step(dt);
        body->setForce("Normal", glm::vec3(0.0f));
        body->setForce("Gravity", body->getMass() * globalAcceleration);
    }

    simTime += dt;

    for (int i = 0; i < bodies.size(); ++i) {
        for (int j = i + 1; j < bodies.size(); ++j) {
            IPhysicsBody* a = bodies[i];
            IPhysicsBody* b = bodies[j];

            if (a->getIsStatic() && b->getIsStatic()) continue;

            if (a->collidesWith(*b)) {
                a->resolveCollisionWith(*b);
            }
        }
    }
}

void Physics::PhysicsSystem::enablePhysics() {
    physicsEnabled = true;
}

void Physics::PhysicsSystem::disablePhysics() {
    physicsEnabled = false;
}

