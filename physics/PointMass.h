#pragma once
#include <glm/glm.hpp>

#include "graphics/interfaces/IPhysicsBody.h"

namespace Physics {
    class PointMass : public IPhysicsBody{
    public:
        glm::vec3 position;
        glm::vec3 velocity = glm::vec3(0.0f);
        glm::vec3 netForce = glm::vec3(0.0f);
        float mass;

        PointMass(float m, glm::vec3 pos = glm::vec3(0.0f))
            : position(pos), mass(m) {}

        void applyForce(const glm::vec3& force) override;
        void step(float deltaTime) override;
        glm::vec3 getPosition() const override {return position;}
        void setPosition(const glm::vec3& pos) override {position = pos;}

        bool collidesWith(const IPhysicsBody& other) const override;

    };

}
