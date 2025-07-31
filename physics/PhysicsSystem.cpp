#include "PhysicsSystem.h"
#include <iostream>
#include <algorithm>
#include "PointMass.h"
#include "solver/OneUnknownSolver.h"

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
    //if (!physicsEnabled) return;

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
glm::vec3 Physics::PhysicsSystem::debugSolveInitialVelocity(
    IPhysicsBody* body,
    float targetDistance,
    float targetTime,
    float dt
) {
    // We’ll solve for a single float: the initial vx
    float elapsed = 0.f;

    // 1) Setter: reset world & apply candidate vx
    auto setVx = [&](float vx0) {
        // reset the entire world to t=0
        this->simTime       = 0.f;
        this->physicsEnabled= true;
        // assume you have a reset() that restores all bodies to their initial poses
        body->setPosition(glm::vec3(0.0f));
        // now apply only to our test body:
        body->setVelocity({vx0, 0.f, 0.f});
        elapsed = 0.f;
    };

    // 2) Runner: integrate until x ≥ targetDistance
    auto runToX = [&]() {
        while (body->getPosition().x < targetDistance) {
            this->step(dt);  // now actually advances
            elapsed += dt;
            if (elapsed > targetTime * 2.f)
                break;  // safety cap
        }
    };

    // 3) Extractor: return how long it took
    auto getTime = [&]() -> float {
        return elapsed;
    };

    // Build a scalar solver: float → float
    OneUnknownSolver<float, float> solver(
        setVx,       // ParamSetter: float vx0
        runToX,      // SimulationRun
        getTime,     // ResultExtractor: float elapsed
        targetTime   // we want elapsed == targetTime
    );

    // bracket guesses for vx (in m/s)
    float guessA = 0.f;
    float guessB = 20.f;

    float vx_solution = solver.solve(guessA, guessB);

    // Return it as a vec3 (only x matters here)
    return { vx_solution, 0.f, 0.f };
}

void Physics::PhysicsSystem::enablePhysics() {
    physicsEnabled = true;
}

void Physics::PhysicsSystem::disablePhysics() {
    physicsEnabled = false;
}

