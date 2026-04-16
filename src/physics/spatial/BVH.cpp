#include "BVH.h"
#include <algorithm>

void BVH::clear() {
    nodes.clear();
}

NodeIndex BVH::allocateNode() {
    nodes.push_back(BVHNode());
    return NodeIndex{static_cast<int>(nodes.size() - 1)};
}

void BVH::build(std::vector<Physics::PhysicsBody*> bodies) {
    BVH::clear();
    if (bodies.empty()) return;
    nodes.reserve(bodies.size() * 2);
    build(bodies, NodeIndex{0}, NodeIndex{static_cast<int>(bodies.size())});
}

NodeIndex BVH::build(std::vector<Physics::PhysicsBody*>& bodies, NodeIndex start, NodeIndex end) {
    NodeIndex nodeIdx = allocateNode();
    BVHNode& node = nodes[nodeIdx.val];
    
    // Base case, leaf node has a body and its own bounds
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

    glm::vec3 centroidMin = bodies[start.val]->getPosition(BodyLock::NOLOCK);
    glm::vec3 centroidMax = bodies[start.val]->getPosition(BodyLock::NOLOCK);
    for (int i = start.val + 1; i < end.val; i++) {
        glm::vec3 pos = bodies[i]->getPosition(BodyLock::NOLOCK);
        centroidMin = glm::min(centroidMin, pos);
        centroidMax = glm::max(centroidMax, pos);
    }

    // Choose the most spread axis to split on
    glm::vec3 extent = centroidMax - centroidMin;
    Axis splitAxis = Axis::X;
    if (extent.y > extent.x) splitAxis = Axis::Y;
    if (extent.z > extent[splitAxis]) splitAxis = Axis::Z;

    // split into 2 group and recurse
    int mid = start.val + (end.val - start.val) / 2; // avoid overflow
    std::nth_element(
        bodies.begin() + start.val,
        bodies.begin() + mid,
        bodies.begin() + end.val,
        [splitAxis](Physics::PhysicsBody* a, Physics::PhysicsBody* b) {
            return a->getPosition(BodyLock::NOLOCK)[splitAxis] < 
                    b->getPosition(BodyLock::NOLOCK)[splitAxis];
        }
    );
    NodeIndex leftIdx = build(bodies, start, NodeIndex{mid});
    NodeIndex rightIdx = build(bodies, NodeIndex{mid}, end);
    Physics::Bounding::AABB mergedBound = nodes[leftIdx.val].bounds;
    mergedBound.expand(nodes[rightIdx.val].bounds);

    // Update internal node bounds according to the children
    node        = nodes[nodeIdx.val]; // Refresh reference after potential vector resize
    node.left   = leftIdx;
    node.right  = rightIdx;
    node.body   = nullptr;
    node.bounds = mergedBound;
    
    return nodeIdx;
}



