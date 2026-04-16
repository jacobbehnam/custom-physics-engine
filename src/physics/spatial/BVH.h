#pragma once

#include <glm/glm.hpp>

#include "../bounding/AABB.h"
#include "NodeIndex.h"
#include "physics/PhysicsBody.h"

struct BVHNode {
    Physics::Bounding::AABB bounds;
    NodeIndex left;
    NodeIndex right;
    Physics::PhysicsBody* body = nullptr;

    bool isLeaf() const {
        return body != nullptr;
    }
};

class BVH {
private:
    std::vector<BVHNode> nodes;

    void clear();
    NodeIndex allocateNode();
    NodeIndex build(std::vector<Physics::PhysicsBody*>& bodies, NodeIndex start, NodeIndex end);
public:
    BVH() = default;
    void build(std::vector<Physics::PhysicsBody*> bodies);
    std::vector<std::pair<Physics::PhysicsBody*, Physics::PhysicsBody*>> getPotentialCollisions() const;
};
