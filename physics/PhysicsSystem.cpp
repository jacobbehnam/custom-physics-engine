#include "PhysicsSystem.h"
#include <iostream>
#include <algorithm>
#include "PointMass.h"

namespace Physics {
    class PointMass;
}

Physics::PhysicsSystem::PhysicsSystem(const glm::vec3 &globalAccel) : globalAcceleration(globalAccel), elapsed(std::chrono::steady_clock::duration::zero()) {}

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
        body->recordFrame(elapsedSeconds());
        body->step(dt);
        body->setForce("Normal", glm::vec3(0.0f));
    }

    for (int i = 0; i < bodies.size(); ++i) {
        for (int j = i + 1; j < bodies.size(); ++j) {
            IPhysicsBody* a = bodies[i];
            IPhysicsBody* b = bodies[j];

            // if (a->isStatic() && b->isStatic()) continue;

            if (a->collidesWith(*b)) {
                a->resolveCollisionWith(*b);
            }
        }
    }
}

void Physics::PhysicsSystem::enablePhysics() {
    physicsEnabled = true;
    startTime = std::chrono::steady_clock::now();
}

void Physics::PhysicsSystem::disablePhysics() {
    physicsEnabled = false;
    auto now = std::chrono::steady_clock::now();
    elapsed += (now - startTime);
}

float Physics::PhysicsSystem::elapsedSeconds() const {
    auto total = elapsed;
    if (physicsEnabled) {
        total += (std::chrono::steady_clock::now() - startTime);
    }
    return std::chrono::duration<float>(total).count();
}

