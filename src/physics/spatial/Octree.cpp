#include "Octree.h"
#include "glm/glm.hpp"
#include "algorithm"
#include <glm/gtx/component_wise.hpp>
#include <cstdint>

void Octree::clear() {
    nodes.clear();
}

Octant Octree::getOctant(NodeIndex nodeIdx, const glm::vec3& pos) const {
    const glm::vec3& center = nodes[nodeIdx.val].center;

    return Octant{static_cast<std::uint8_t>(
        (pos.x >= center.x ? Octant::X_MASK : 0) |
        (pos.y >= center.y ? Octant::Y_MASK : 0) |
        (pos.z >= center.z ? Octant::Z_MASK : 0)
    )};
}

NodeIndex Octree::allocateNode(const glm::vec3& center, float halfSize) {
    nodes.push_back({center, halfSize});
    return NodeIndex{static_cast<int>(nodes.size() - 1)};
}

void Octree::insert(NodeIndex nodeIndex, Physics::PhysicsBody* body) {
    OctreeNode& node        = nodes[nodeIndex.val];
    glm::vec3 nodeCenter    = node.center;
    glm::vec3 bodyPos       = body->getPosition(BodyLock::NOLOCK);
    float bodyMass          = body->getMass(BodyLock::NOLOCK);

    // Node is empty, put the body here
    if (node.body == nullptr && node.totalMass == 0.0f) {
        node.body       = body;
        node.massCenter = bodyPos;
        node.totalMass  = bodyMass;
        return;
    }

    // Since this node now contains this body, update
    float newMass   = node.totalMass + bodyMass;
    node.massCenter = (
        node.massCenter * node.totalMass + 
        bodyPos         * bodyMass
    ) / newMass;
    node.totalMass  = newMass;

    // Leaf already has a body, has to push it down
    if (node.body != nullptr) {
        Physics::PhysicsBody* existingBody = node.body;

        // Clear first
        node.body = nullptr;

        // Allocate children
        float childHalfSize = node.halfSize * 0.5f;
        for (uint8_t oct = 0; oct < 8; ++oct) {
            glm::vec3 childCenter = nodeCenter + childHalfSize * glm::vec3(
                (oct & Octant::X_MASK) ? 1.0f : -1.0f,
                (oct & Octant::Y_MASK) ? 1.0f : -1.0f,
                (oct & Octant::Z_MASK) ? 1.0f : -1.0f
            );

            node.children[oct] = allocateNode(childCenter, childHalfSize);
        }

        // Push it down
        Octant existingOctant = getOctant(nodeIndex, existingBody->getPosition(BodyLock::NOLOCK));
        insert(node.children[existingOctant.val], existingBody);
    }

    // Insert body into tree
    Octant octant = Octree::getOctant(nodeIndex, bodyPos);
    insert(node.children[octant.val], body);
}

void Octree::build(const std::vector<Physics::PhysicsBody*>& bodies) {
    Octree::clear();
    if (bodies.empty()) return;

    // Find the center
    glm::vec3 min = bodies[0]->getPosition(BodyLock::NOLOCK);
    glm::vec3 max = bodies[0]->getPosition(BodyLock::NOLOCK);
    for (const auto& body : bodies) {
        glm::vec3 pos = body->getPosition(BodyLock::NOLOCK);
        min = glm::min(min, pos);
        max = glm::max(max, pos);
    }

    glm::vec3 center = (min + max) * 0.5f;
    float halfSize = glm::compMax(max - center) + 1.0f; // Avoid points right on the edge

    // Add root node
    NodeIndex root = allocateNode(center, halfSize);

    for (const auto& body : bodies) {
        insert(root, body);
    }
}
