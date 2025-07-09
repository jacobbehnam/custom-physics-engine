#include "PointMass.h"
#include <iostream>

void Physics::PointMass::applyForce(const glm::vec3 &force) {
    netForce += force;
}

void Physics::PointMass::step(float dt) {
    velocity = velocity + (netForce/mass)*dt;
    position = position + velocity * dt + (netForce/(2 * mass)) * dt * dt;
}

bool Physics::PointMass::collidesWith(const IPhysicsBody &other) const {
    float threshold = 0.01f;
    return glm::distance(position, other.getPosition()) <= threshold;
}
