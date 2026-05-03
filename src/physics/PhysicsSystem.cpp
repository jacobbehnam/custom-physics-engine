#include "PhysicsSystem.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include "PointMass.h"
#include "physics/utils/ThermalUtils.h"

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

Physics::PhysicsBody* Physics::PhysicsSystem::getBodyById(uint32_t id) const {
    std::lock_guard<std::mutex> lock(bodiesMutex);

    for (PhysicsBody* body : bodies) {
        if (body->getID() == id) return body;
    }
    return nullptr;
}

std::optional<std::vector<ObjectSnapshot>> Physics::PhysicsSystem::fetchLatestSnapshot(float renderSimTime) {
    if (!snapshotReady.load(std::memory_order_acquire))
        return std::nullopt;

    std::lock_guard<std::mutex> lk(snapshotMutex);

    if (currentSnapshots.empty()) {
        return std::nullopt;
    }

    if (previousSnapshots.empty() || previousSnapshots.size() != currentSnapshots.size()) {
        return currentSnapshots;
    }

    float t0 = previousSnapshots[0].time;
    float t1 = currentSnapshots[0].time;

    if (!std::isfinite(t0) || !std::isfinite(t1) || t1 <= t0) {
        return currentSnapshots;
    }

    if (renderSimTime <= t0) {
        return previousSnapshots;
    }
    if (renderSimTime >= t1) {
        return currentSnapshots;
    }

    float alpha = (renderSimTime - t0) / (t1 - t0);

    std::vector<ObjectSnapshot> out;
    out.reserve(currentSnapshots.size());
    for (size_t i = 0; i < currentSnapshots.size(); ++i) {
        const auto &A = previousSnapshots[i];
        const auto &B = currentSnapshots[i];

        if (A.body != B.body) {
            out.push_back(B);
            continue;
        }

        ObjectSnapshot C;
        C.body        = A.body;
        C.time        = renderSimTime;
        C.position    = glm::mix(A.position, B.position, alpha);
        C.velocity    = glm::mix(A.velocity, B.velocity, alpha);
        C.temperature = glm::mix(A.temperature, B.temperature, alpha);

        out.push_back(C);
    }

    return out;
}

void Physics::PhysicsSystem::physicsLoop() {
    constexpr float kBaseDt = 1.0f / 1000.0f;
    constexpr int kMaxCatchUpSteps = 8;
    constexpr float kMaxAdaptiveDt = 86400.0f;

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

        std::vector<ObjectSnapshot> localSnaps;
        {
            std::lock_guard<std::mutex> lock(bodiesMutex);
            if (accumulator >= kBaseDt) {
                const int neededSteps = static_cast<int>(std::ceil(accumulator / kBaseDt));
                const int stepsToRun = std::max(1, std::min(neededSteps, kMaxCatchUpSteps));
                const float dt = std::min(std::max(accumulator / static_cast<float>(stepsToRun), kBaseDt), kMaxAdaptiveDt);

                for (int i = 0; i < stepsToRun && accumulator >= kBaseDt; ++i) {
                    if (step(dt)) {
                        accumulator = 0.0f;
                        break;
                    }
                    accumulator -= std::min(dt, accumulator);
                }

                if (dt >= kMaxAdaptiveDt && accumulator >= kMaxAdaptiveDt * kMaxCatchUpSteps) {
                    accumulator = 0.0f;
                }
            }

            localSnaps.reserve(bodies.size());
            for (auto* body : bodies) {
                localSnaps.push_back({ body,simTime, body->getPosition(BodyLock::LOCK), body->getVelocity(BodyLock::LOCK), static_cast<float>(body->getThermalProperties(BodyLock::LOCK).tempK) });
            }

            std::lock_guard<std::mutex> lk(snapshotMutex);
            previousSnapshots = std::move(currentSnapshots);
            currentSnapshots = std::move(localSnaps);
            snapshotReady.store(true, std::memory_order_release);
        }
    }
}

void Physics::PhysicsSystem::addBody(PhysicsBody *body) {
    std::lock_guard<std::mutex> lock(bodiesMutex);
    body->setForce("Gravity", static_cast<float>(body->getMass(BodyLock::LOCK)) * getGlobalAcceleration(), BodyLock::LOCK);
    body->setForce("Normal", glm::vec3(0.0f), BodyLock::LOCK);
    bodies.push_back(body);
}

void Physics::PhysicsSystem::removeBody(PhysicsBody *body) {
    std::lock_guard<std::mutex> lock(bodiesMutex);
    auto it = std::remove(bodies.begin(), bodies.end(), body);
    if (it != bodies.end()) {
        bodies.erase(it, bodies.end());
        resetState.erase(body);
        {
            std::lock_guard<std::mutex> snapshotLock(snapshotMutex);
            auto removeSnapshotForBody = [body](std::vector<ObjectSnapshot>& snapshots) {
                snapshots.erase(
                    std::remove_if(snapshots.begin(), snapshots.end(), [body](const ObjectSnapshot& snapshot) {
                        return snapshot.body == body;
                    }),
                    snapshots.end()
                );
            };
            removeSnapshotForBody(currentSnapshots);
            removeSnapshotForBody(previousSnapshots);
            if (currentSnapshots.empty() && previousSnapshots.empty()) {
                snapshotReady.store(false, std::memory_order_relaxed);
            }
        }
    } else {
        std::cerr << "[PhysicsSystem] Warning: Tried to remove a body not in the system.\n";
    }
}

void Physics::PhysicsSystem::advancePhysics(float dt) {
    float targetTime = simTime + dt;
    std::vector<PhysicsBody*> collidableBodies;

    PhysicsSystem::octree.build(bodies);
    for (auto body : bodies) {
        std::unique_lock<std::mutex> guard = body->lockState();
        if (body->getCollider() != nullptr) {
            collidableBodies.push_back(body);
        }

        glm::vec3 nBodyGravity  = PhysicsSystem::octree.computeForce(body, getGravitationalConstant());
        glm::vec3 globalGravity = static_cast<float>(body->getMass(BodyLock::NOLOCK)) * getGlobalAcceleration();
        glm::vec3 totalGravity  = nBodyGravity + globalGravity;
        
        body->setForce("Normal", glm::vec3(0.0f), BodyLock::NOLOCK);
        body->setForce("Gravity", totalGravity, BodyLock::NOLOCK);

        if (simTime == 0.0f) {
            body->recordFrame(0.0f, BodyLock::NOLOCK);

            if (resetState.find(body) == resetState.end()) {
                body->withFrames(BodyLock::NOLOCK, [this, body](const std::vector<ObjectSnapshot>& fr) {
                    if (!fr.empty()) resetState[body] = fr.front();
                });
            }
        }

        ThermalProperties props = body->getThermalProperties(BodyLock::NOLOCK);
        const double area = body->getSurfaceArea();
        const double mass = body->getMass(BodyLock::NOLOCK);
        const double ambientTemp = getAmbientTemperature();
        const double proximityRadiation = PhysicsSystem::octree.computeHeat(body);

        Physics::Thermal::integrateTemperature(props, mass, dt, [&](double tempK) {
            ThermalProperties tmp = props;
            tmp.tempK = tempK;
            return Physics::Thermal::convectionHeatRate(tmp, area, ambientTemp)
                + Physics::Thermal::ambientRadiationHeatRate(tmp, area, ambientTemp)
                + Physics::Thermal::externalHeatFluxRate(tmp, area)
                + proximityRadiation
                + tmp.internalHeatPower;
        });
        if (std::isfinite(props.tempK)) {
            body->setThermalProperty(props, BodyLock::NOLOCK);
        }

        if (!body->getIsStatic(BodyLock::NOLOCK)) {
            body->step(dt, BodyLock::NOLOCK);
        }
        body->recordFrame(targetTime, BodyLock::NOLOCK);
    }

    // Broad phase
    BVH bvh;
    bvh.build(collidableBodies);
    for (const auto[a, b] : bvh.getPotentialCollisions()) {
        // Narrow phase
        if (a->getIsStatic(BodyLock::LOCK) && b->getIsStatic(BodyLock::LOCK)) continue;

        if (a->collidesWith(*b)) {
            a->resolveCollisionWith(dt, *b);
        }
    }

    stepCount++;
    simTime = targetTime;
}

bool Physics::PhysicsSystem::step(float dt) {
    if (solver && solver->stepFrame()) {
        std::cout << "Solver Converged!" << std::endl;

        float finalDuration = this->simTime - (dt * 0.5f); // ensures no extra step is made due to floating point errors

        for (auto body : bodies) {
            body->withFrames(BodyLock::LOCK, [this, body](const std::vector<ObjectSnapshot>& frames) {
                if (!frames.empty()) {
                    // The first frame is always t=0 for the current trajectory
                    resetState[body] = frames.front();
                }
            });
        }
        reset();

        float bakeTimer = 0.0f;

        while (bakeTimer < finalDuration) {
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
    solver = router.makeSolver(body, knowns, unknown);

    if (solver) {
        std::cout << "Solver Started: " << unknown << std::endl;
        physicsEnabled = true;
    } else {
        std::cerr << "Solver Error: No recipe found for " << unknown << std::endl;
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

void Physics::PhysicsSystem::clearRuntimeState() {
    std::lock_guard<std::mutex> bodiesLock(bodiesMutex);
    resetState.clear();
    solver.reset();
    stepCount.store(0);
    simTime = 0.0f;
    {
        std::lock_guard<std::mutex> snapshotLock(snapshotMutex);
        currentSnapshots.clear();
        previousSnapshots.clear();
    }
    snapshotReady.store(false, std::memory_order_relaxed);
}

void Physics::PhysicsSystem::enablePhysics() {
    physicsEnabled.store(true);
    snapshotReady.store(false, std::memory_order_relaxed); // so we don't read from the stale buffer
}

void Physics::PhysicsSystem::disablePhysics() {
    physicsEnabled.store(false);
}
