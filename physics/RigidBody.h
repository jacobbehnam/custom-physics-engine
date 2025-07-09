#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "graphics/interfaces/IPhysicsBody.h"

namespace Physics {

    class RigidBody : public IPhysicsBody {
    public:
        RigidBody(float mass, glm::vec3 pos);

        void applyForce(const glm::vec3& force) override;
        void step(float dt) override;

        bool isStatic() const;

        glm::vec3 getPosition() override {return position;}
        void setPosition(const glm::vec3& pos) override {position = pos;}

    private:
        glm::vec3 position;
        glm::vec3 velocity = glm::vec3(0.0f);

        glm::vec3 netForce = glm::vec3(0.0f);

        float mass;
    };

}
