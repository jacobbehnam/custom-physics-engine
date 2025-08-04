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
    if (running.exchange(true)) return;
    physicsThread = std::thread(&PhysicsSystem::physicsLoop, this);
}

void Physics::PhysicsSystem::stop() {
    running = false;
    stepDone.notify_all();
    waitForStop();
}

void Physics::PhysicsSystem::waitForStop() {
    if (physicsThread.joinable())
        physicsThread.join();
}

std::optional<std::vector<ObjectSnapshot>> Physics::PhysicsSystem::fetchLatestSnapshot() {
    if (!snapshotReady.load() || !physicsEnabled.load()) return std::nullopt;

    std::lock_guard<std::mutex> lk(snapshotMutex);
    int snapshotIndex = currentSnapshot;
    return snapshotBuf[snapshotIndex];
}

void Physics::PhysicsSystem::physicsLoop() {
    constexpr float dt = 1.0f / 1000.0f;
    int writeBuf = 0;

    while (running.load()) {
        {
            std::lock_guard<std::mutex> lock(bodiesMutex);
            if (!physicsEnabled.load()) { continue; }
            step(dt);
        }

        {
            auto &buf = snapshotBuf[writeBuf];
            buf.clear();
            std::lock_guard<std::mutex> lk(snapshotMutex);
            buf.reserve(bodies.size());
            for (auto *body : bodies) {
                ObjectSnapshot snap;
                snap.body = body;
                snap.time = simTime;
                snap.position = body->getPosition();
                snap.velocity = body->getVelocity();
                buf.push_back(snap);
            }
        }
        {
            std::lock_guard<std::mutex> lk(snapshotMutex);
            currentSnapshot = writeBuf;
            snapshotReady = true;
        }
        stepDone.notify_one();
        writeBuf = 1 - writeBuf;

        std::this_thread::sleep_for(std::chrono::duration<float>(dt));
    }
}


void Physics::PhysicsSystem::addBody(IPhysicsBody *body) {
    bodies.push_back(body);
    body->recordFrame(simTime);
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
            //std::cout << solver->current << std::endl;
        } else {
            physicsEnabled = false;
            std::cout << "finished" << std::endl;
        }
    }
}
void Physics::PhysicsSystem::debugSolveInitialVelocity(
    IPhysicsBody* body,
    float targetDistance,
    float targetTime
) {

    const auto resetState = body->getAllFrames().front();
    // 1) Setter: reset world & apply candidate vx
    auto setVx = [this, body, resetState](float vx0) {
        // reset the entire world to t=0
        this->simTime       = 0.f;
        // assume you have a reset() that restores all bodies to their initial poses
        reset(body, resetState);
        // now apply only to our test body:
        body->setVelocity({vx0, 0.f, 0.f});
    };

    // 2) Runner: integrate until x ≥ targetDistance
    auto runToX = [this, body, targetDistance]()->bool {
        if (body->getAllFrames().back().position.x > targetDistance) {
            body->setPosition(glm::vec3(targetDistance, 0.0f, 0.0f));
            return true;
        }
        if (simTime >= 10) {
            return true;
        }
        return false;
    };

    // 3) Extractor: return how long it took
    auto getTime = [this, body, targetDistance]() -> float {
        const auto& frames = body->getAllFrames();
        int N = (int)frames.size();
        if (N < 2) return simTime;  // fallback

        return MathUtils::interpolateCrossing<float, float>(
            frames[N-2], frames[N-1],
            targetDistance,
            [](const ObjectSnapshot &snap){ return snap.position.x; },
            [](const ObjectSnapshot &snap){ return snap.time; }
            );
    };

    // Build a scalar solver: float → float
    solver = new OneUnknownSolver<float, float> (
        setVx,       // ParamSetter: float vx0
        runToX,      // SimulationRun
        getTime,     // ResultExtractor: float elapsed
        targetTime   // we want elapsed == targetTime
    );
}

void Physics::PhysicsSystem::enablePhysics() {
    physicsEnabled.store(true);
}

void Physics::PhysicsSystem::disablePhysics() {
    physicsEnabled.store(false);
}

