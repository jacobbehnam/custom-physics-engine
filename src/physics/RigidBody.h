#pragma once
#include <mutex>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "bounding/ICollider.h"
#include "physics/PhysicsBody.h"

namespace Physics {

    class RigidBody : public PhysicsBody {
    public:
        RigidBody(uint32_t id, float mass, ICollider* collider, glm::vec3 pos = glm::vec3(0.0f), bool isStatic = false);
        explicit RigidBody(uint32_t id, ICollider* collider, glm::vec3 pos = glm::vec3(0.0f), bool isStatic = true); // static objects dont need mass

        void step(float dt, BodyLock lock) override;

        void recordFrame(float t, BodyLock lock) override;
        void loadFrame(const ObjectSnapshot &snapshot, BodyLock lock) override;

        ICollider* getCollider() const override { return collider; }

        bool collidesWith(const PhysicsBody &other) const override;
        bool collidesWithPointMass(const PointMass &pm) const override;
        bool collidesWithRigidBody(const RigidBody &rb) const override;

        bool resolveCollisionWith(PhysicsBody &other) override;
        bool resolveCollisionWithPointMass(PointMass &pm) override;
        bool resolveCollisionWithRigidBody(RigidBody &rb) override;

        ICollider* collider = nullptr;
    };

}
