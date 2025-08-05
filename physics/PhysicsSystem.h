#pragma once
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <optional>

#include "RigidBody.h"
#include "solver/OneUnknownSolver.h"

namespace Physics {

    class PhysicsSystem {
    public:
        explicit PhysicsSystem(const glm::vec3& globalAccel = glm::vec3(0.0f, -9.81f, 0.0f));
        ~PhysicsSystem();

        // thread control
        void start();
        void stop();
        void waitForStop();

        void addBody(PhysicsBody* body);
        void removeBody(PhysicsBody* body);

        void step(float dt);

        void enablePhysics();
        void disablePhysics();

        glm::vec3 getGlobalAcceleration() const { return globalAcceleration; }
        void setGlobalAcceleration(const glm::vec3& newAcceleration) { globalAcceleration = newAcceleration; }

        std::optional<std::vector<ObjectSnapshot>> fetchLatestSnapshot(float renderSimTime);

        void debugSolveInitialVelocity(PhysicsBody* body, float targetDistance, float targetTime);
        void reset(PhysicsBody* body, const ObjectSnapshot &state) {
            body->clearAllFrames();
            body->loadFrame(state, BodyLock::LOCK);
            simTime = 0.0f;
        }

    private:
        void physicsLoop();

        OneUnknownSolver<float, float>* solver = nullptr;
        ObjectSnapshot resetState{};

        glm::vec3 globalAcceleration;
        std::vector<PhysicsBody*> bodies;

        std::atomic<bool> physicsEnabled{false};
        float simTime = 0.0f;

        // threading
        std::thread physicsThread;
        std::mutex bodiesMutex;
        std::atomic<bool> threadRunning{false};

        std::mutex snapshotMutex;
        std::vector<ObjectSnapshot> currentSnapshots;
        std::vector<ObjectSnapshot> previousSnapshots;
        std::atomic<bool> snapshotReady{false};
    };

}
