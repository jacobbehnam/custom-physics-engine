#pragma once

#include "../PhysicsBody.h"
#include <glm/glm.hpp>
#include <vector>

struct NodeIndex {
    int val = -1;
};

struct OctreeNode {
    glm::vec3 center;
    float halfSize;
    NodeIndex children[8];

    // Leaf nodes only
    Physics::PhysicsBody* body = nullptr;

    // Aggregated properties (center, mass)
    glm::vec3 massCenter;
    float totalMass;

    bool isLeaf() const {
        return body != nullptr;
    }
};

class Octree {
private:
    std::vector<OctreeNode> nodes;
public:
    Octree(const glm::vec3& center, float halfSize);
    void insert(Physics::PhysicsBody* body);
};
