#pragma once
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

#include "graphics/interfaces/IPhysicsBody.h"

namespace Physics {
    class PointMass : public IPhysicsBody{
    public:
        glm::vec3 position;
        glm::vec3 velocity = glm::vec3(0.0f);
        glm::vec3 netForce = glm::vec3(0.0f);
        float mass;

        PointMass(float m, glm::vec3 pos = glm::vec3(0.0f), bool isStatic = false);
        PointMass(glm::vec3 pos = glm::vec3(0.0f), bool isStatic = true); // static objects dont need mass

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
        float getMass() const override { return mass; }
        void setMass(float newMass) override;

        bool getIsStatic() const override { return isStatic; }

        void setWorldTransform(const glm::mat4& M) override { worldMatrix = M; }
        void recordFrame(float t) override { frames.push_back( {this, t, position, velocity}); }
        const std::vector<ObjectSnapshot> &getAllFrames() const override { return frames; }
        void clearAllFrames() override { frames.clear(); }
        void loadFrame(const ObjectSnapshot &snapshot) override;

        bool collidesWith(const IPhysicsBody& other) const override;
        bool collidesWithPointMass(const PointMass& pm) const override;
        bool collidesWithRigidBody(const RigidBody &rb) const override;

        bool resolveCollisionWith(IPhysicsBody &other) override;
        bool resolveCollisionWithPointMass(PointMass &pm) override;
        bool resolveCollisionWithRigidBody(RigidBody &rb) override;
    private:
        bool isStatic;
        std::map<std::string, glm::vec3> forces;
        glm::mat4 worldMatrix = glm::mat4(1.0f);
        std::vector<ObjectSnapshot> frames;
    };

}
