#include "Octree.h"
#include "physics/Constants.h"
#include "physics/PhysicsSystem.h"
#include <bit>
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
    OctreeNode* node        = &nodes[nodeIndex.val];
    glm::vec3 nodeCenter    = node->center;
    glm::vec3 bodyPos       = body->getPosition(BodyLock::NOLOCK);
    float bodyMass          = body->getMass(BodyLock::NOLOCK);
    float childHalfSize     = node->halfSize * 0.5f;

    // Node is empty, put the body here
    if (node->body == nullptr && node->totalMass == 0.0f) {
        node->body       = body;
        node->massCenter = bodyPos;
        node->totalMass  = bodyMass;
        return;
    }

    // Since this node now contains this body, update
    float newMass       = node->totalMass + bodyMass;
    node->massCenter    = (
        node->massCenter    * node->totalMass + 
        bodyPos             * bodyMass
    ) / newMass;
    node->totalMass     = newMass;

    Physics::PhysicsBody* existingBody = node->body;

    auto insertBody = [&](Physics::PhysicsBody* b) {
        glm::vec3 bPos = b->getPosition(BodyLock::NOLOCK);
        Octant bOct = Octree::getOctant(nodeIndex, bPos);

        // Could change due to vector resize during recursion 
        node = &nodes[nodeIndex.val];
        if (node->children[bOct.val].isEmpty()) {
            glm::vec3 childCenter = nodeCenter + childHalfSize * glm::vec3(
                (bOct.val & Octant::X_MASK) ? 1.0f : -1.0f,
                (bOct.val & Octant::Y_MASK) ? 1.0f : -1.0f,
                (bOct.val & Octant::Z_MASK) ? 1.0f : -1.0f
            );
            NodeIndex childNode = allocateNode(childCenter, childHalfSize);
            node = &nodes[nodeIndex.val]; // Refresh reference after potential vector resize
            node->children[bOct.val] = childNode;
            node->childMask |= (1 << bOct.val);
        }
        insert(node->children[bOct.val], b);
    };

    if (existingBody != nullptr) {
        // Clear and Push the existing body down the tree
        node->body = nullptr;
        insertBody(existingBody);
    }
    // Insert the new body
    insertBody(body);
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

glm::vec3 Octree::computeForce(Physics::PhysicsBody* body, float G) {
    if (nodes.empty() || body == nullptr) {
        return glm::vec3(0.0f);
    }

    glm::vec3 totalForce(0.0f);
    glm::vec3 bodyPos   = body->getPosition(BodyLock::NOLOCK);
    float bodyMass      = body->getMass(BodyLock::NOLOCK);

    // Max depth of 64, thats already
    NodeIndex stack[64]; 
    int stackPtr = 0;
    stack[stackPtr++] = OctreeNode::rootIndex();

    while (stackPtr > 0) {
        NodeIndex currentIdx = stack[--stackPtr];
        const OctreeNode& node = nodes[currentIdx.val];

        // Empty region
        if (node.totalMass == 0.0f) continue;
        
        glm::vec3 dist = node.massCenter - body->getPosition(BodyLock::NOLOCK);
        float distSq = glm::dot(dist, dist);
        float softeningDistSq = distSq + Constants::SOFTENING_SQ;

        if (node.isLeaf()) {
            if (node.body == body) continue; // Skip self

            float invDist = 1.0f / sqrt(softeningDistSq);
            float invDist3 = invDist * invDist * invDist;

            float force = (G * bodyMass * node.totalMass) * invDist3;
            totalForce += force * dist;
        } else {
            float widthSq = node.halfSize * node.halfSize * 4.0f;

            if (widthSq < Constants::THETA_SQ * distSq) {
                // Treat as a single body
                float invDist = 1.0f / sqrt(softeningDistSq);
                float invDist3 = invDist * invDist * invDist;

                float force = (G * bodyMass * node.totalMass) * invDist3;
                totalForce += force * dist;
            } else {
                // Add valid children to stack
                std::uint8_t childMask = node.childMask;
                while (childMask) {
                    int childOctant = std::countr_zero(childMask);
                    stack[stackPtr++] = node.children[childOctant];
                    childMask &= (childMask - 1);
                }
            }
            
        }
    }
    return totalForce;
}
