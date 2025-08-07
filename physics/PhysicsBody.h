#pragma once
#include <atomic>
#include <string>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <mutex>

namespace Physics {
    class PointMass;
    class RigidBody;
}

struct ObjectSnapshot;

enum class BodyLock {
    LOCK,
    NOLOCK
};

namespace Physics {
    class PhysicsBody {
    public:
        virtual ~PhysicsBody() = default;

        virtual void step(float dt, BodyLock lock) = 0;
        virtual void recordFrame(float t, BodyLock lock) = 0;
        virtual void loadFrame(const ObjectSnapshot& snapshot, BodyLock lock) = 0;

        std::unique_lock<std::mutex> lockState() const { return std::unique_lock(stateMutex); }

        void applyForce(const glm::vec3& force);
        void setForce(const std::string& name, const glm::vec3& force, BodyLock lock);
        glm::vec3 getForce(const std::string& name, BodyLock lock) const;
        glm::vec3 getNetForce(BodyLock lock) const;
        std::map<std::string, glm::vec3> getAllForces(BodyLock lock) const;

        glm::vec3 getPosition(BodyLock lock) const;
        void setPosition(const glm::vec3& pos, BodyLock lock);
        glm::vec3 getVelocity(BodyLock lock) const;
        void setVelocity(const glm::vec3& vel, BodyLock lock);
        float getMass(BodyLock lock) const;
        void setMass(float newMass, BodyLock lock);
        bool getIsStatic(BodyLock lock) const;
        void setIsStatic(bool newStatic, BodyLock lock);

        glm::mat4 getWorldTransform(BodyLock lock) const;
        void setWorldTransform(const glm::mat4& M, BodyLock lock);
        void setGlobalAccelerationRef(std::atomic<glm::vec3>& globalAccRef) { globalAccelPtr = &globalAccRef; }
        std::vector<ObjectSnapshot> getAllFrames(BodyLock lock) const;
        void clearAllFrames(BodyLock lock);

        // Uses double dispatch
        virtual bool collidesWith(const PhysicsBody& other) const = 0;
        virtual bool collidesWithPointMass(const PointMass& pm) const = 0;
        virtual bool collidesWithRigidBody(const RigidBody& rb) const = 0;

        virtual bool resolveCollisionWith(PhysicsBody& other) = 0;
        virtual bool resolveCollisionWithPointMass(PointMass& pm) = 0;
        virtual bool resolveCollisionWithRigidBody(RigidBody& rb) = 0;
    protected:
        mutable std::mutex stateMutex;
        std::vector<ObjectSnapshot> frames;
    private:
        bool isStatic = false;

        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 velocity = glm::vec3(0.0f);
        glm::vec3 netForce = glm::vec3(0.0f);
        std::map<std::string, glm::vec3> forces;

        glm::mat4 worldMatrix = glm::mat4(1.0f);
        std::atomic<glm::vec3>* globalAccelPtr = nullptr;

        float mass = 1.0f;
    };
}

struct ObjectSnapshot {
    Physics::PhysicsBody* body;
    float time;
    glm::vec3 position;
    glm::vec3 velocity;
};