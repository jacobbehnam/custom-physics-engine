#pragma once
#include <vector>
#include "RigidBody.h"
#include "solver/OneUnknownSolver.h"

namespace Physics {

    class PhysicsSystem {
    public:
        explicit PhysicsSystem(const glm::vec3& globalAccel = glm::vec3(0.0f, -9.81f, 0.0f));

        void addBody(IPhysicsBody* body) { bodies.push_back(body); }
        void removeBody(IPhysicsBody* body);

        void step(float dt);

        void enablePhysics();
        void disablePhysics();

        glm::vec3 getGlobalAcceleration() const { return globalAcceleration; }
        void setGlobalAcceleration(const glm::vec3& newAcceleration) { globalAcceleration = newAcceleration; }

        void debugSolveInitialVelocity(
            IPhysicsBody* body,
            float targetDistance,
            float targetTime
        );

        float dt = 0.0f;
        OneUnknownSolver<float, float>* solver = nullptr;
        float thing = 20.0f;

    private:
        glm::vec3 globalAcceleration;
        std::vector<IPhysicsBody*> bodies;

        bool physicsEnabled = true;
        float simTime = 0.0f;
    };

}
