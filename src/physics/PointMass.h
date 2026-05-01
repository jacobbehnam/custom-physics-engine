#pragma once
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <mutex>

#include "physics/PhysicsBody.h"

namespace Physics {
    class PointMass : public PhysicsBody{
    public:
        explicit PointMass(uint32_t id, float m, glm::vec3 pos = glm::vec3(0.0f), bool isStatic = false);
        explicit PointMass(uint32_t id, glm::vec3 pos = glm::vec3(0.0f), bool isStatic = true); // static objects dont need mass

        void applyImpulse(const glm::vec3& impulse, BodyLock lock);

        void setMass(float newMass, BodyLock lock) override;
        void setThermalProperty(const ThermalProperties& newProps, BodyLock lock) override;

        void step(float dt, BodyLock lock) override;

        void recordFrame(float t, BodyLock lock) override;
        void loadFrame(const ObjectSnapshot &snapshot, BodyLock lock) override;

        bool collidesWith(const PhysicsBody& other) const override;
        bool collidesWithPointMass(const PointMass& pm) const override;
        bool collidesWithRigidBody(const RigidBody &rb) const override;

        bool resolveCollisionWith(float dt, PhysicsBody &other) override;
        bool resolveCollisionWithPointMass(float dt, PointMass &pm) override;
        bool resolveCollisionWithRigidBody(float dt, RigidBody &rb) override;
    private:
        void recomputeSurfaceArea();
    };

}
