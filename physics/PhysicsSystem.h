#pragma once
#include <vector>
#include "RigidBody.h"
#include "solver/OneUnknownSolver.h"

namespace Physics {

    class PhysicsSystem {
    public:
        explicit PhysicsSystem(const glm::vec3& globalAccel = glm::vec3(0.0f, -9.81f, 0.0f));

        void addBody(IPhysicsBody* body);
        void removeBody(IPhysicsBody* body);

        void step(float dt);

        void enablePhysics();
        void disablePhysics();

        glm::vec3 getGlobalAcceleration() const { return globalAcceleration; }
        void setGlobalAcceleration(const glm::vec3& newAcceleration) { globalAcceleration = newAcceleration; }

        void debugSolveInitialVelocity(IPhysicsBody* body, float targetDistance, float targetTime);
        void reset(IPhysicsBody* body, const ObjectSnapshot &state) {
            body->clearAllFrames();
            body->loadFrame(state);
            simTime = 0.0f;
        }

    private:
        OneUnknownSolver<float, float>* solver = nullptr;

        glm::vec3 globalAcceleration;
        std::vector<IPhysicsBody*> bodies;

        bool physicsEnabled = false;
        float simTime = 0.0f;
    };

}
