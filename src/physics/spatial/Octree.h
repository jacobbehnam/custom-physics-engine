#pragma once

#include "../PhysicsBody.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

struct NodeIndex {
    int val = -1;

    bool isEmpty() const {
        return val == -1;
    }
};

struct Octant {
    std::uint8_t val; // 0-7 representing the octant

    static constexpr std::uint8_t X_MASK = 1 << 0;
    static constexpr std::uint8_t Y_MASK = 1 << 1;
    static constexpr std::uint8_t Z_MASK = 1 << 2;
};

struct OctreeNode {
    glm::vec3 center;
    float halfSize;
    NodeIndex children[8];
    uint8_t childMask = 0; // Bitmask to track which children exist

    // Leaf nodes only
    Physics::PhysicsBody* body = nullptr;

    // Aggregated properties (center, mass)
    glm::vec3 massCenter;
    float totalMass = 0.0f;

    bool isLeaf() const {
        return childMask == 0;
    }

    static NodeIndex rootIndex() {
        return NodeIndex{0};
    }
};

class Octree {
private:
    std::vector<OctreeNode> nodes;
    void clear();
    NodeIndex allocateNode(const glm::vec3& center, float halfSize);
    void insert(NodeIndex nodeIndex, Physics::PhysicsBody* body);
    Octant getOctant(NodeIndex nodeIdx, const glm::vec3& pos) const;
public:
    Octree() = default;
    glm::vec3 computeForce(Physics::PhysicsBody* body);
    void build(const std::vector<Physics::PhysicsBody*>& bodies);
};
