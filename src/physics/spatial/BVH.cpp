#include "BVH.h"

void BVH::clear() {
    nodes.clear();
}

NodeIndex BVH::allocateNode() {
    nodes.push_back(BVHNode());
    return NodeIndex{static_cast<int>(nodes.size() - 1)};
}

void BVH::build(const std::vector<Physics::PhysicsBody*> bodies) {
    BVH::clear();
    if (bodies.empty()) return;
    nodes.reserve(bodies.size() * 2);
    build(bodies, NodeIndex{0}, NodeIndex{static_cast<int>(bodies.size())});
}

NodeIndex BVH::build(const std::vector<Physics::PhysicsBody*>& bodies, NodeIndex start, NodeIndex end) {
    NodeIndex nodeIdx = allocateNode();
    BVHNode& node = nodes[nodeIdx.val];
    
    if (end.val - start.val == 1) {
        Physics::PhysicsBody* body  = bodies[start.val];
        glm::vec3 pos               = body->getPosition(BodyLock::NOLOCK);

        node.body   = body;
        node.bounds = Physics::Bounding::AABB(pos, glm::vec3(0.5f)); // Maybe real bound or just AABB (not sure yet)

        return nodeIdx;
    }
}



