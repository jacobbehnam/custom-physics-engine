#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "graphics/interfaces/ICollider.h"
#include "graphics/interfaces/IPhysicsBody.h"

namespace Physics {

    class RigidBody : public IPhysicsBody {
    public:
        RigidBody(float mass, glm::vec3 pos, ICollider* collider);

        void setForce(const std::string &name, const glm::vec3 &force) override;

        glm::vec3 getForce(const std::string &name) const override { return forces.find(name)->second; }

        std::map<std::string, glm::vec3> getAllForces() const override { return forces; };

        void applyForce(const glm::vec3& force) override;
        void step(float dt) override;

        bool isStatic() const;

        glm::vec3 getPosition() const override { return position; }
        void setPosition(const glm::vec3& pos) override { position = pos; }
        glm::vec3 getVelocity() const override { return velocity; }
        void setVelocity(const glm::vec3 &vel) override { velocity = vel; }

        void setWorldTransform(const glm::mat4& M) override { worldMatrix = M; }
        void recordFrame(float t) override {} // TODO

        bool collidesWith(const IPhysicsBody &other) const override;
        bool collidesWithPointMass(const PointMass &pm) const override;
        bool collidesWithRigidBody(const RigidBody &rb) const override;

        bool resolveCollisionWith(IPhysicsBody &other) override;
        bool resolveCollisionWithPointMass(PointMass &pm) override;
        bool resolveCollisionWithRigidBody(RigidBody &rb) override;

        ICollider* collider = nullptr;
    private:
        glm::vec3 position;
        glm::vec3 velocity = glm::vec3(0.0f);
        glm::vec3 netForce = glm::vec3(0.0f);
        std::map<std::string, glm::vec3> forces;

        glm::mat4 worldMatrix = glm::mat4(1.0f);
        std::vector<ObjectSnapshot> frames;

        float mass;
    };

}
