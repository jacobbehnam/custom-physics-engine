#pragma once
#include <string>
#include <glm/glm.hpp>
#include <map>

namespace Physics {
    class PointMass;
    class RigidBody;
}

struct ObjectSnapshot {
    float time;
    glm::vec3 position;
    glm::vec3 velocity;
};

class IPhysicsBody {
public:
    virtual ~IPhysicsBody() = default;
    virtual void applyForce(const glm::vec3& force) = 0;
    virtual void setForce(const std::string& name, const glm::vec3& force) = 0;
    virtual glm::vec3 getForce(const std::string& name) const = 0;
    virtual std::map<std::string, glm::vec3> getAllForces() const = 0;
    virtual void step(float dt) = 0;

    virtual glm::vec3 getPosition() const = 0;
    virtual void setPosition(const glm::vec3& pos) = 0;
    virtual glm::vec3 getVelocity() const = 0;
    virtual void setVelocity(const glm::vec3& vel) = 0;

    virtual void setWorldTransform(const glm::mat4& M) = 0;
    virtual void recordFrame(float t) = 0;
    virtual const std::vector<ObjectSnapshot>& getAllFrames() const = 0;

    // Uses double dispatch
    virtual bool collidesWith(const IPhysicsBody& other) const = 0;
    virtual bool collidesWithPointMass(const Physics::PointMass& pm) const = 0;
    virtual bool collidesWithRigidBody(const Physics::RigidBody& rb) const = 0;

    virtual bool resolveCollisionWith(IPhysicsBody& other) = 0;
    virtual bool resolveCollisionWithPointMass(Physics::PointMass& pm) = 0;
    virtual bool resolveCollisionWithRigidBody(Physics::RigidBody& rb) = 0;
};
