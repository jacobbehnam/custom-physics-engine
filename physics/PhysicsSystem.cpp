#include "PhysicsSystem.h"
#include <iostream>
#include <algorithm>
#include "PointMass.h"
#include "graphics/utils/MathUtils.h"
#include "solver/OneUnknownSolver.h"

namespace Physics {
    class PointMass;
}

Physics::PhysicsSystem::PhysicsSystem(const glm::vec3 &globalAccel) : globalAcceleration(globalAccel), router(*this) {}

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

std::optional<std::vector<ObjectSnapshot>> Physics::PhysicsSystem::fetchLatestSnapshot(float renderSimTime) {
    if (!snapshotReady.load(std::memory_order_acquire))
        return std::nullopt;

    std::lock_guard<std::mutex> lk(snapshotMutex);

    // first call ever: no previous to interpolate against
    if (previousSnapshots.empty()) {
        previousSnapshots = currentSnapshots;
        return currentSnapshots;
    }

    // grab the two timestamps (they’re all the same within each frame)
    float t0 = previousSnapshots[0].time;
    float t1 = currentSnapshots[0].time;

    // outside the bracket → just clamp
    if (renderSimTime <= t0) {
        previousSnapshots = currentSnapshots;
        return previousSnapshots;
    }
    if (renderSimTime >= t1) {
        previousSnapshots = currentSnapshots;
        return currentSnapshots;
    }

    // compute blend factor α in (0,1)
    float alpha = (renderSimTime - t0) / (t1 - t0);

    // interpolate each ObjectSnapshot
    std::vector<ObjectSnapshot> out;
    out.reserve(currentSnapshots.size());
    for (size_t i = 0; i < currentSnapshots.size(); ++i) {
        const auto &A = previousSnapshots[i];
        const auto &B = currentSnapshots[i];

        ObjectSnapshot C;
        C.body     = A.body;                  // same pointer/ID
        C.time     = renderSimTime;           // stamped with the render time
        C.position = glm::mix(A.position, B.position, alpha);
        C.velocity = glm::mix(A.velocity, B.velocity, alpha);

        out.push_back(C);
    }

    // slide window: next time, previous = what we just returned
    previousSnapshots = currentSnapshots;
    return out;
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
            accumulator = 0.0f;
            lastTime = std::chrono::high_resolution_clock::now();
            continue;
        }

        accumulator += frameTime * getSimSpeed();

        std::vector<PhysicsBody*> localBodies;
        {
            std::lock_guard<std::mutex> lock(bodiesMutex);
            while (accumulator >= dt) {
                if (step(dt)) {
                    accumulator = 0.0f;
                    break;
                }
                accumulator -= dt;
            }
            localBodies = bodies;
        }

        std::vector<ObjectSnapshot> localSnaps;
        localSnaps.reserve(localBodies.size());
        {
            std::lock_guard<std::mutex> lk(snapshotMutex);
            for (auto* body : localBodies) {
                localSnaps.push_back({ body,simTime, body->getPosition(BodyLock::LOCK), body->getVelocity(BodyLock::LOCK) });
            }

            currentSnapshots = std::move(localSnaps);
        }
        snapshotReady.store(true, std::memory_order_release);
    }
}

void Physics::PhysicsSystem::addBody(PhysicsBody *body) {
    std::lock_guard<std::mutex> lock(bodiesMutex);
    body->setForce("Gravity", body->getMass(BodyLock::LOCK) * getGlobalAcceleration(), BodyLock::LOCK);
    body->setForce("Normal", glm::vec3(0.0f), BodyLock::LOCK);
    bodies.push_back(body);
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

void Physics::PhysicsSystem::advancePhysics(float dt) {
    stepCount++;

    for (auto body : bodies) {
        std::unique_lock<std::mutex> guard = body->lockState();
        if (body->getIsStatic(BodyLock::NOLOCK))
            continue;

        if (simTime == 0.0f)
            body->recordFrame(0.0f, BodyLock::NOLOCK);

        body->step(dt, BodyLock::NOLOCK);

        simTime = stepCount.load() * dt;
        body->recordFrame(simTime, BodyLock::NOLOCK);

        if (resetState.find(body) == resetState.end()) {
            resetState[body] = body->getAllFrames(BodyLock::NOLOCK).front();
        }

        body->setForce("Normal", glm::vec3(0.0f), BodyLock::NOLOCK);
        body->setForce("Gravity", body->getMass(BodyLock::NOLOCK) * getGlobalAcceleration(), BodyLock::NOLOCK);
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
}

bool Physics::PhysicsSystem::step(float dt) {
    if (solver && solver->stepFrame()) {
        std::cout << "Solver Converged!" << std::endl;

        float bakeTimer = 0.0f;
        while (bakeTimer < solverTargetTime) {
            advancePhysics(dt);
            bakeTimer += dt;
        }

        solver = nullptr;
        physicsEnabled = false;
        return true;
    }

    advancePhysics(dt);
    return false;
}

void Physics::PhysicsSystem::solveProblem(PhysicsBody* body, const std::unordered_map<std::string, double> &knowns, const std::string &unknown) {
    if (knowns.find("T") != knowns.end()) {
        solverTargetTime = static_cast<float>(knowns.at("T"));
    } else {
        solverTargetTime = 10.0f; // Default fallback
    }
    auto decision = router.routeProblem(body, knowns, unknown);
    if (decision.mode == SolverMode::SIMULATE) {
        // TODO
    } else if (decision.mode == SolverMode::SOLVE) {
        solver = std::move(decision.solver);
    } else {
        std::cout << "null" << std::endl;
    }
}

void Physics::PhysicsSystem::reset() {
    stepCount.store(0);
    simTime = 0.0f;
    for (auto [body, initialState] : resetState) {
        body->clearAllFrames(BodyLock::LOCK);
        body->loadFrame(initialState, BodyLock::LOCK);
    }
}

void Physics::PhysicsSystem::enablePhysics() {
    physicsEnabled.store(true);
    snapshotReady.store(false, std::memory_order_relaxed); // so we don't read from the stale buffer
}

void Physics::PhysicsSystem::disablePhysics() {
    physicsEnabled.store(false);
}

