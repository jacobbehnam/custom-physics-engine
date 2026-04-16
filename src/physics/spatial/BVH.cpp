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
        Physics::PhysicsBody* body              = bodies[start.val];
        glm::vec3 pos                           = body->getPosition(BodyLock::NOLOCK);
        Physics::Bounding::ICollider* collider  = body->getCollider();

        std::unique_ptr<Physics::Bounding::ICollider> worldCollider = 
            collider->getTransformed(body->getWorldTransform(BodyLock::NOLOCK));

        glm::vec3 minCorner = worldCollider->getAABBMin();
        glm::vec3 maxCorner = worldCollider->getAABBMax();

        glm::vec3 center        = (minCorner + maxCorner) * 0.5f;
        glm::vec3 halfExtents   = (maxCorner - minCorner) * 0.5f;

        node.body   = body;
        node.bounds = Physics::Bounding::AABB(center, halfExtents);

        return nodeIdx;
    }
}



