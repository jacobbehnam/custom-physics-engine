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
                // Perfectly Elastic Collision
                std::cout << "hi" << std::endl;
                glm::vec3 normal = glm::normalize(b->position - a->position);
                glm::vec3 relativeVelocity = b->velocity - a->velocity;

                // Project relative velocity onto the normal
                float velocityAlongNormal = glm::dot(relativeVelocity, normal);

                // If they're moving apart, no collision to resolve
                if (velocityAlongNormal <= 0) return;

                float m1 = a->mass;
                float m2 = b->mass;

                // Compute impulse scalar for perfectly elastic collision
                float impulse = (2.0f * velocityAlongNormal) / (m1 + m2);

                // Apply impulses to both velocities
                a->velocity += impulse * m2 * normal;
                b->velocity -= impulse * m1 * normal;
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

