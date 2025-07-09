#include "PointMass.h"

void Physics::PointMass::applyForce(const glm::vec3 &force) {
    netForce += force;
}

void Physics::PointMass::step(float dt) {
    velocity = velocity + (netForce/mass)*dt;
    position = position + velocity * dt + (netForce/(2 * mass)) * dt * dt;
}
