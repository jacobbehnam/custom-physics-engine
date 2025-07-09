#pragma once
#include <glm/glm.hpp>

class IPhysicsBody {
public:
    virtual void applyForce(const glm::vec3& force) = 0;
    virtual void step(float dt) = 0;
    virtual glm::vec3 getPosition() = 0;
    virtual void setPosition(const glm::vec3& pos) = 0;
    virtual ~IPhysicsBody() = default;
};
