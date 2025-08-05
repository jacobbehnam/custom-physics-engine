#include "PhysicsSystem.h"
#include <iostream>
#include <algorithm>
#include "PointMass.h"
#include "graphics/utils/MathUtils.h"
#include "solver/OneUnknownSolver.h"

namespace Physics {
    class PointMass;
}

Physics::PhysicsSystem::PhysicsSystem(const glm::vec3 &globalAccel) : globalAcceleration(globalAccel) {}

Physics::PhysicsSystem::~PhysicsSystem() {
    stop();
    waitForStop();
}

void Physics::PhysicsSystem::start() {
    if (threadRunning.exchange(true)) return;
    physicsThread = std::thread(&PhysicsSystem::physicsLoop, this);
}

void Physics::PhysicsSystem::stop() {
    threadRunning = false;
    waitForStop();
}

void Physics::PhysicsSystem::waitForStop() {
    if (physicsThread.joinable())
        physicsThread.join();
}

std::optional<std::vector<ObjectSnapshot>> Physics::PhysicsSystem::fetchLatestSnapshot() {
    if (!snapshotReady.load(std::memory_order_acquire) || !physicsEnabled.load())
        return std::nullopt;

    std::lock_guard<std::mutex> lk(snapshotMutex);
    return currentSnapshots;
}

void Physics::PhysicsSystem::physicsLoop() {
    constexpr float dt = 1.0f / 1000.0f;

    float accumulator = 0.0f;
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (threadRunning.load()) {
        auto now = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        if (!physicsEnabled.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        accumulator += frameTime;

        std::vector<PhysicsBody*> localBodies;
        {
            std::lock_guard<std::mutex> lock(bodiesMutex);
            while (accumulator >= dt) {
                step(dt);
                simTime += dt;
                accumulator -= dt;
            }
            localBodies = bodies;
        }

        std::vector<ObjectSnapshot> localSnaps;
        localSnaps.reserve(localBodies.size());
        std::lock_guard<std::mutex> lk(snapshotMutex);
        for (auto* body : localBodies) {
            localSnaps.push_back({ body,simTime, body->getPosition(BodyLock::LOCK), body->getVelocity(BodyLock::LOCK) });
        }

        currentSnapshots = std::move(localSnaps);
        snapshotReady.store(true, std::memory_order_release);
    }
}


void Physics::PhysicsSystem::addBody(PhysicsBody *body) {
    std::lock_guard<std::mutex> lock(bodiesMutex);
    bodies.push_back(body);
    //body->recordFrame(simTime);
}

void Physics::PhysicsSystem::removeBody(PhysicsBody *body) {
    std::lock_guard<std::mutex> lock(bodiesMutex);
    auto it = std::remove(bodies.begin(), bodies.end(), body);
    if (it != bodies.end()) {
        bodies.erase(it, bodies.end());
    } else {
        std::cerr << "[PhysicsSystem] Warning: Tried to remove a body not in the system.\n";
    }
}

void Physics::PhysicsSystem::step(float dt) {
    for (auto body : bodies) {
        std::unique_lock<std::mutex> guard = body->lockState();
        if (body->getIsStatic(BodyLock::NOLOCK))
            continue;
        body->recordFrame(simTime, BodyLock::NOLOCK);
        body->step(dt, BodyLock::NOLOCK);
        body->setForce("Normal", glm::vec3(0.0f), BodyLock::NOLOCK);
        body->setForce("Gravity", body->getMass(BodyLock::NOLOCK) * globalAcceleration, BodyLock::NOLOCK);
    }

    for (int i = 0; i < bodies.size(); ++i) {
        for (int j = i + 1; j < bodies.size(); ++j) {
            PhysicsBody* a = bodies[i];
            PhysicsBody* b = bodies[j];

            if (a->getIsStatic(BodyLock::LOCK) && b->getIsStatic(BodyLock::LOCK)) continue;

            if (a->collidesWith(*b)) {
                a->resolveCollisionWith(*b);
            }
        }
    }
    if (solver) {
        if (!solver->stepFrame()) {
            // still solving—optionally display current guess:
            //std::cout << solver->current << std::endl;
        } else {
            physicsEnabled = false;
            std::cout << "finished" << std::endl;
        }
    }
}
// void Physics::PhysicsSystem::debugSolveInitialVelocity(
//     PhysicsBody* body,
//     float targetDistance,
//     float targetTime
// ) {
//
//     const auto resetState = body->getAllFrames().front();
//     // 1) Setter: reset world & apply candidate vx
//     auto setVx = [this, body, resetState](float vx0) {
//         // reset the entire world to t=0
//         this->simTime       = 0.f;
//         // assume you have a reset() that restores all bodies to their initial poses
//         reset(body, resetState);
//         // now apply only to our test body:
//         body->setVelocity({vx0, 0.f, 0.f});
//     };
//
//     // 2) Runner: integrate until x ≥ targetDistance
//     auto runToX = [this, body, targetDistance]()->bool {
//         if (body->getAllFrames().back().position.x > targetDistance) {
//             body->setPosition(glm::vec3(targetDistance, 0.0f, 0.0f));
//             return true;
//         }
//         if (simTime >= 10) {
//             return true;
//         }
//         return false;
//     };
//
//     // 3) Extractor: return how long it took
//     auto getTime = [this, body, targetDistance]() -> float {
//         const auto& frames = body->getAllFrames();
//         int N = (int)frames.size();
//         if (N < 2) return simTime;  // fallback
//
//         return MathUtils::interpolateCrossing<float, float>(
//             frames[N-2], frames[N-1],
//             targetDistance,
//             [](const ObjectSnapshot &snap){ return snap.position.x; },
//             [](const ObjectSnapshot &snap){ return snap.time; }
//             );
//     };
//
//     // Build a scalar solver: float → float
//     solver = new OneUnknownSolver<float, float> (
//         setVx,       // ParamSetter: float vx0
//         runToX,      // SimulationRun
//         getTime,     // ResultExtractor: float elapsed
//         targetTime   // we want elapsed == targetTime
//     );
// }

void Physics::PhysicsSystem::enablePhysics() {
    physicsEnabled.store(true);
    snapshotReady.store(false, std::memory_order_relaxed); // so we don't read from the stale buffer
}

void Physics::PhysicsSystem::disablePhysics() {
    physicsEnabled.store(false);
}

