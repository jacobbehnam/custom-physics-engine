#include "PhysicsSystem.h"
#include <iostream>
#include <algorithm>
#include "PointMass.h"
#include "solver/OneUnknownSolver.h"

namespace Physics {
    class PointMass;
}

Physics::PhysicsSystem::PhysicsSystem(const glm::vec3 &globalAccel) : globalAcceleration(globalAccel) {
}

void Physics::PhysicsSystem::removeBody(IPhysicsBody *body) {
    auto it = std::remove(bodies.begin(), bodies.end(), body);
    if (it != bodies.end()) {
        bodies.erase(it, bodies.end());
    } else {
        std::cerr << "[PhysicsSystem] Warning: Tried to remove a body not in the system.\n";
    }
}

void Physics::PhysicsSystem::step(float dt) {
    //if (!physicsEnabled) return;
    this->dt = dt;

    for (auto body : bodies) {
        if (body->getIsStatic())
            continue;
        body->recordFrame(simTime);
        body->step(dt);
        body->setForce("Normal", glm::vec3(0.0f));
        //body->setForce("Gravity", body->getMass() * globalAcceleration);
        body->setForce("Gravity", glm::vec3(0.0f));
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
    if (solver) {
        if (!solver->stepFrame()) {
            // still solving—optionally display current guess:
            std::cout << solver->current << std::endl;
        } else {
            std::cout << "finished" << std::endl;
        }
    }
}
void Physics::PhysicsSystem::debugSolveInitialVelocity(
    IPhysicsBody* body,
    float targetDistance,
    float targetTime
) {

    // 1) Setter: reset world & apply candidate vx
    auto setVx = [this, body](float vx0) {
        // reset the entire world to t=0
        this->simTime       = 0.f;
        this->physicsEnabled= true;
        // assume you have a reset() that restores all bodies to their initial poses
        body->setPosition(glm::vec3(0.0f));
        // now apply only to our test body:
        body->setVelocity({vx0, 0.f, 0.f});
    };

    // 2) Runner: integrate until x ≥ targetDistance
    auto runToX = [=, body]()->bool {
        return body->getAllFrames().back().position.x > targetDistance;
    };

    // 3) Extractor: return how long it took
    auto getTime = [&]() -> float {
        return simTime;
    };

    // Build a scalar solver: float → float
    solver = new OneUnknownSolver<float, float> (
        setVx,       // ParamSetter: float vx0
        runToX,      // SimulationRun
        getTime,     // ResultExtractor: float elapsed
        targetTime   // we want elapsed == targetTime
    );
    solver->init(1.0f, 20.0f);
}

void Physics::PhysicsSystem::enablePhysics() {
    physicsEnabled = true;
}

void Physics::PhysicsSystem::disablePhysics() {
    physicsEnabled = false;
}

