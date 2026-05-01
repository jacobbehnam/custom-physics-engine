#pragma once
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <optional>

#include "RigidBody.h"
#include "physics/Constants.h"
#include "solver/OneUnknownSolver.h"
#include "solver/ProblemRouter.h"
#include "spatial/Octree.h"
#include "spatial/BVH.h"
#include "solver/VectorRootSolver.h"

namespace Physics {
    class PhysicsSystem {
    public:
        explicit PhysicsSystem(const glm::vec3& globalAccel = glm::vec3(0.0f, -Constants::STANDARD_GRAVITY, 0.0f));
        ~PhysicsSystem();

        // thread control
        void start();
        void stop();
        void waitForStop();

        void addBody(PhysicsBody* body);
        void removeBody(PhysicsBody* body);
        PhysicsBody* getBodyById(uint32_t id) const;

        bool step(float dt);

        void enablePhysics();
        void disablePhysics();
        bool isPhysicsEnabled() const { return physicsEnabled.load(); }

        glm::vec3 getGlobalAcceleration() const { return globalAcceleration.load(); }
        void setGlobalAcceleration(const glm::vec3& newAcceleration) { globalAcceleration.store(newAcceleration); }
        float getSimSpeed() const { return simSpeed.load(); }
        void setSimSpeed(float newSpeed) { simSpeed.store(newSpeed); }
        double getGravitationalConstant() const { return gravitationalConstant.load(); }
        void setGravitationalConstant(double newG) { gravitationalConstant.store(newG); }

        float getAmbientTemperature() const { return ambientTemperature.load(); }
        void setAmbientTemperature(float newTemp) { ambientTemperature.store(newTemp); }

        std::optional<std::vector<ObjectSnapshot>> fetchLatestSnapshot(float renderSimTime);

        const ProblemRouter* getRouter() const { return &router; }
        void solveProblem(PhysicsBody* body, const std::unordered_map<std::string, double>& knowns, const std::string& unknown = "");
        void reset();

        float simTime = 0.0f; // TODO move

    private:
        void physicsLoop();
        void advancePhysics(float dt);

        ProblemRouter router;
        std::unique_ptr<ISolver> solver = nullptr;
        float solverTargetTime = 10.0f;
        std::unordered_map<PhysicsBody*, ObjectSnapshot> resetState{};

        Octree octree;

        std::atomic<glm::vec3> globalAcceleration;
        std::atomic<float> simSpeed{1.0f};
        std::atomic<double> gravitationalConstant{Constants::G};
        std::atomic<float> ambientTemperature{293.15f};
        std::atomic<long long> stepCount{0};
        std::vector<PhysicsBody*> bodies;

        std::atomic<bool> physicsEnabled{false};

        // threading
        std::thread physicsThread;
        mutable std::mutex bodiesMutex;
        std::atomic<bool> threadRunning{false};

        std::mutex snapshotMutex;
        std::vector<ObjectSnapshot> currentSnapshots;
        std::vector<ObjectSnapshot> previousSnapshots;
        std::atomic<bool> snapshotReady{false};
    };

}
