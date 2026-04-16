#include "BVH.h"
#include <algorithm>
#include <glm/gtx/component_wise.hpp>

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

std::vector<std::pair<Physics::PhysicsBody*, Physics::PhysicsBody*>> BVH::getPotentialCollisions() const {
    std::vector<std::pair<Physics::PhysicsBody*, Physics::PhysicsBody*>> potentialCollisions;
    if (nodes.empty() || nodes[NodeIndex::rootIndex().val].isLeaf()) return potentialCollisions;

    // A micro optimize is to store as raw pointer
    // since we dont allocate more nodes so its guarantee to be safe
    std::vector<std::pair<NodeIndex, NodeIndex>> stack;
    stack.reserve(512);
    stack.emplace_back(NodeIndex::rootIndex(), NodeIndex::rootIndex());

    while (!stack.empty()) {
        auto [idxA, idxB] = stack.back();
        stack.pop_back();

        const BVHNode& a = nodes[idxA.val];
        const BVHNode& b = nodes[idxB.val];

        // Check internal node
        if (idxA.val == idxB.val) {
            if (!a.isLeaf()) {
                stack.emplace_back(a.left, a.left);
                stack.emplace_back(a.right, a.right);
                
                const BVHNode& leftChild = nodes[a.left.val];
                const BVHNode& rightChild = nodes[a.right.val];
                
                if (leftChild.bounds.intersectsAABB(rightChild.bounds)) {
                    stack.emplace_back(a.left, a.right);
                }
            }
            continue;
        }
        
        // Check leaf node
        if (a.isLeaf() && b.isLeaf()) {
            potentialCollisions.emplace_back(a.body, b.body);
            continue;
        }

        // Split on the bigger volume
        bool splitA;
        if (a.isLeaf()) {
            splitA = false;
        } else if (b.isLeaf()) {
            splitA = true;
        } else {
            glm::vec3 extentA = a.bounds.getAABBMax() - a.bounds.getAABBMin();
            glm::vec3 extentB = b.bounds.getAABBMax() - b.bounds.getAABBMin();
            splitA = glm::compMul(extentA) > glm::compMul(extentB);
        }

        if (splitA) {
            const BVHNode& aLeft = nodes[a.left.val];
            const BVHNode& aRight = nodes[a.right.val];

            if (aLeft.bounds.intersectsAABB(b.bounds)) {
                stack.emplace_back(a.left, idxB);
            }
            if (aRight.bounds.intersectsAABB(b.bounds)) {
                stack.emplace_back(a.right, idxB);
            }
        } else {
            const BVHNode& bLeft = nodes[b.left.val];
            const BVHNode& bRight = nodes[b.right.val];

            if (a.bounds.intersectsAABB(bLeft.bounds)) {
                stack.emplace_back(idxA, b.left);
            }
            if (a.bounds.intersectsAABB(bRight.bounds)) {
                stack.emplace_back(idxA, b.right);
            }
        }
    }

    return potentialCollisions;
}


