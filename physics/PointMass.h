#pragma once
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <mutex>

#include "PhysicsBody.h"

namespace Physics {
    class PointMass : public PhysicsBody{
    public:
        explicit PointMass(float m, glm::vec3 pos = glm::vec3(0.0f), bool isStatic = false);
        explicit PointMass(glm::vec3 pos = glm::vec3(0.0f), bool isStatic = true); // static objects dont need mass

        void applyImpulse(const glm::vec3& impulse, BodyLock lock);

        void step(float dt, BodyLock lock) override;

        void recordFrame(float t, BodyLock lock) override;
        void loadFrame(const ObjectSnapshot &snapshot, BodyLock lock) override;

        bool collidesWith(const PhysicsBody& other) const override;
        bool collidesWithPointMass(const PointMass& pm) const override;
        bool collidesWithRigidBody(const RigidBody &rb) const override;

        bool resolveCollisionWith(PhysicsBody &other) override;
        bool resolveCollisionWithPointMass(PointMass &pm) override;
        bool resolveCollisionWithRigidBody(RigidBody &rb) override;
    };

}
