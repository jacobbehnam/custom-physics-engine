#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Physics {

    class RigidBody {
    public:
        RigidBody(float mass, glm::vec3 pos);

        void applyForce(const glm::vec3& force);
        void step(float dt);

        bool isStatic() const;

        glm::vec3 getPosition() {return position;}

    private:
        glm::vec3 position;
        glm::vec3 velocity = glm::vec3(0.0f);

        glm::vec3 netForce = glm::vec3(0.0f);

        float mass;
    };

}