#pragma once
#include <mutex>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "graphics/interfaces/ICollider.h"
#include "graphics/interfaces/IPhysicsBody.h"

namespace Physics {

    class RigidBody : public IPhysicsBody {
    public:
        RigidBody(float mass, ICollider* collider, glm::vec3 pos = glm::vec3(0.0f), bool isStatic = false);
        RigidBody(ICollider* collider, glm::vec3 pos = glm::vec3(0.0f), bool isStatic = true); // static objects dont need mass

        void setForce(const std::string &name, const glm::vec3 &force) override;
        glm::vec3 getForce(const std::string &name) const override;
        std::map<std::string, glm::vec3> getAllForces() const override;
        void applyForce(const glm::vec3& force) override;
        void step(float dt) override;

        bool getIsStatic() const override;

        glm::vec3 getPosition() const override;
        void setPosition(const glm::vec3& pos) override;
        glm::vec3 getVelocity() const override;
        void setVelocity(const glm::vec3 &vel) override;
        float getMass() const override;
        void setMass(float newMass) override;

        void setWorldTransform(const glm::mat4& M) override;
        void recordFrame(float t) override;
        const std::vector<ObjectSnapshot> &getAllFrames() const override;
        void clearAllFrames() override;
        void loadFrame(const ObjectSnapshot &snapshot) override;

        bool collidesWith(const IPhysicsBody &other) const override;
        bool collidesWithPointMass(const PointMass &pm) const override;
        bool collidesWithRigidBody(const RigidBody &rb) const override;

        bool resolveCollisionWith(IPhysicsBody &other) override;
        bool resolveCollisionWithPointMass(PointMass &pm) override;
        bool resolveCollisionWithRigidBody(RigidBody &rb) override;

        ICollider* collider = nullptr;
    private:
        mutable std::mutex stateMutex;
        bool isStatic;

        glm::vec3 position;
        glm::vec3 velocity = glm::vec3(0.0f);
        glm::vec3 netForce = glm::vec3(0.0f);
        std::map<std::string, glm::vec3> forces;

        glm::mat4 worldMatrix = glm::mat4(1.0f);
        std::vector<ObjectSnapshot> frames;

        float mass;
    };

}
