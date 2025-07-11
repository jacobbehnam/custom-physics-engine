#pragma once
#include <glm/glm.hpp>


namespace Physics {
    class PointMass;
    class RigidBody;
}

class IPhysicsBody {
public:
    virtual ~IPhysicsBody() = default;
    virtual void applyForce(const glm::vec3& force) = 0;
    virtual void step(float dt) = 0;
    virtual glm::vec3 getPosition() const = 0;
    virtual void setPosition(const glm::vec3& pos) = 0;

    // Uses double dispatch
    virtual bool collidesWith(const IPhysicsBody& other) const = 0;
    virtual bool collidesWithPointMass(const Physics::PointMass& pm) const = 0;
    virtual bool collidesWithRigidBody(const Physics::RigidBody& rb) const = 0;

    virtual bool resolveCollisionWith(IPhysicsBody& other) = 0;
    virtual bool resolveCollisionWithPointMass(Physics::PointMass& pm) = 0;
    virtual bool resolveCollisionWithRigidBody(Physics::RigidBody& rb) = 0;
};
