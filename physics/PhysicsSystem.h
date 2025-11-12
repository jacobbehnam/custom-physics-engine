#pragma once
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <optional>

#include "RigidBody.h"
#include "solver/OneUnknownSolver.h"
#include "solver/ProblemRouter.h"
#include "solver/VectorRootSolver.h"

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

        bool step(float dt);

        void enablePhysics();
        void disablePhysics();

        glm::vec3 getGlobalAcceleration() const { return globalAcceleration.load(); }
        void setGlobalAcceleration(const glm::vec3& newAcceleration) { globalAcceleration.store(newAcceleration); }
        float getSimSpeed() const { return simSpeed.load(); }
        void setSimSpeed(float newSpeed) { simSpeed.store(newSpeed); }

        std::optional<std::vector<ObjectSnapshot>> fetchLatestSnapshot(float renderSimTime);

        void solveProblem(PhysicsBody* body, const std::unordered_map<std::string, double>& knowns, const std::string& unknown = "");
        void debugSolveInitialVelocity(PhysicsBody* body, const glm::vec3& targetPosition, float maxSimTime);
        void reset();

        float simTime = 0.0f; // TODO move

    private:
        void physicsLoop();

        ProblemRouter router;
        std::unique_ptr<VectorRootSolver<glm::vec3, glm::vec3>> solver = nullptr;
        std::unordered_map<PhysicsBody*, ObjectSnapshot> resetState{};

        std::atomic<glm::vec3> globalAcceleration;
        std::atomic<float> simSpeed{1.0f};
        std::atomic<long long> stepCount{0};
        std::vector<PhysicsBody*> bodies;

        std::atomic<bool> physicsEnabled{false};

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
