#pragma once
#include <vector>
#include "RigidBody.h"

namespace Physics {

    class PhysicsSystem {
    public:
        explicit PhysicsSystem(const glm::vec3& globalAccel = glm::vec3(0.0f, -9.81f, 0.0f));

        void addBody(IPhysicsBody* body);

        void step(float dt);

        void enablePhysics() {physicsEnabled = true;}
        void disablePhysics() {physicsEnabled = false;}

    private:
        glm::vec3 globalAcceleration;
        std::vector<IPhysicsBody*> bodies;

        bool physicsEnabled = false;
    };

}