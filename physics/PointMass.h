#pragma once
#include <unordered_map>
#include <glm/glm.hpp>

#include "graphics/interfaces/IPhysicsBody.h"

namespace Physics {
    class PointMass : public IPhysicsBody{
    public:
        glm::vec3 position;
        glm::vec3 velocity = glm::vec3(0.0f);
        glm::vec3 netForce = glm::vec3(0.0f);
        float mass;

        PointMass(float m, glm::vec3 pos = glm::vec3(0.0f));

        void applyForce(const glm::vec3& force) override;
        void setForce(const std::string &name, const glm::vec3 &force) override;
        glm::vec3 getForce(const std::string &name) const override { return forces.find(name)->second; }
        std::map<std::string, glm::vec3> getAllForces() const override { return forces; }
        void applyImpulse(const glm::vec3& impulse);

        void step(float deltaTime) override;
        glm::vec3 getPosition() const override { return position; }
        void setPosition(const glm::vec3& pos) override { position = pos; }
        glm::vec3 getVelocity() const override { return velocity; }
        void setVelocity(const glm::vec3 &vel) override { velocity = vel; }

        bool collidesWith(const IPhysicsBody& other) const override;
        bool collidesWithPointMass(const PointMass& pm) const override;
        bool collidesWithRigidBody(const RigidBody &rb) const override;

        bool resolveCollisionWith(IPhysicsBody &other) override;
        bool resolveCollisionWithPointMass(PointMass &pm) override;
        bool resolveCollisionWithRigidBody(RigidBody &rb) override;
    private:
        std::map<std::string, glm::vec3> forces;
    };

}
