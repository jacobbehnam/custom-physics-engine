#include "PhysicsSystem.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include "PointMass.h"
#include "math/MathUtils.h"
#include "solver/OneUnknownSolver.h"

#include <glm/gtc/constants.hpp>

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
        C.temperature = glm::mix(A.temperature, B.temperature, alpha);

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
                localSnaps.push_back({ body,simTime, body->getPosition(BodyLock::LOCK), body->getVelocity(BodyLock::LOCK), static_cast<float>(body->getThermalProperties(BodyLock::LOCK).tempK) });
            }

            currentSnapshots = std::move(localSnaps);
        }
        snapshotReady.store(true, std::memory_order_release);
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
    } else {
        std::cerr << "[PhysicsSystem] Warning: Tried to remove a body not in the system.\n";
    }
}

void Physics::PhysicsSystem::advancePhysics(float dt) {
    float targetTime = stepCount.load() * dt + dt; // the time we will be at after this step
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

        if (body->getIsStatic(BodyLock::NOLOCK))
            continue;

        // Thermal updates (ambient convection & radiation)
        ThermalProperties props = body->getThermalProperties(BodyLock::NOLOCK);
        const double area = body->getSurfaceArea();
        const double mass = body->getMass(BodyLock::NOLOCK);
        const double t_amb = getAmbientTemperature();
        
        // Q_conv = h * A * (T_amb - T)
        double q_conv = props.heatTransferCoeff * area * (t_amb - props.tempK);
        // Q_rad = e * sigma * A * (T_amb^4 - T^4)
        constexpr double STEFAN_BOLTZMANN = 5.670374419e-8;
        double t_amb_4 = t_amb * t_amb * t_amb * t_amb;
        double t_obj_4 = props.tempK * props.tempK * props.tempK * props.tempK;
        double q_rad = props.emissivity * STEFAN_BOLTZMANN * area * (t_amb_4 - t_obj_4);
        
        double q_total = q_conv + q_rad;

        // Proximity radiation from all other bodies (O(N log N) using Octree)
        double q_rad_proximity = PhysicsSystem::octree.computeHeat(body);
        q_total += q_rad_proximity;

        // dT = Q * dt / (m * c)
        if (mass > 0.0 && props.specificHeat > 0.0f && std::isfinite(props.tempK) && std::isfinite(q_total)) {
            const double deltaT = (q_total * dt) / (mass * props.specificHeat);
            props.tempK = std::clamp(props.tempK + deltaT, 0.0, 1.0e8);
            body->setThermalProperty(props, BodyLock::NOLOCK);
        }

        if (simTime == 0.0f) {
            body->recordFrame(0.0f, BodyLock::NOLOCK);

            if (resetState.find(body) == resetState.end()) {
                body->withFrames(BodyLock::NOLOCK, [this, body](const std::vector<ObjectSnapshot>& fr) {
                    if (!fr.empty()) resetState[body] = fr.front();
                });
            }
        }

        body->step(dt, BodyLock::NOLOCK);
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

void Physics::PhysicsSystem::enablePhysics() {
    physicsEnabled.store(true);
    snapshotReady.store(false, std::memory_order_relaxed); // so we don't read from the stale buffer
}

void Physics::PhysicsSystem::disablePhysics() {
    physicsEnabled.store(false);
}
