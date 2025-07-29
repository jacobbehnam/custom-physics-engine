#pragma once
#include <vector>
#include "RigidBody.h"
#include <chrono>

namespace Physics {

    class PhysicsSystem {
    public:
        explicit PhysicsSystem(const glm::vec3& globalAccel = glm::vec3(0.0f, -9.81f, 0.0f));

        void addBody(IPhysicsBody* body) { bodies.push_back(body); }
        void removeBody(IPhysicsBody* body);

        void step(float dt);

        void enablePhysics();
        void disablePhysics();
        float elapsedSeconds() const;

    private:
        glm::vec3 globalAcceleration;
        std::vector<IPhysicsBody*> bodies;

        bool physicsEnabled = false;
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::duration elapsed;
    };

}