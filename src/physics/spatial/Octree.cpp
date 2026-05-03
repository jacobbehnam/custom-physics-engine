#include "Octree.h"
#include "physics/Constants.h"
#include "physics/PhysicsSystem.h"
#include "physics/utils/ThermalUtils.h"
#include <bit>
#include <glm/gtx/component_wise.hpp>
#include <cstdint>

namespace {
constexpr float kMinNodeHalfSize = 0.001f;
constexpr std::size_t kTraversalStackReserve = 512;
constexpr double kMinRadiationDistanceSq = 0.0001;

bool containsPosition(const OctreeNode& node, const glm::vec3& position) {
    const glm::vec3 delta = glm::abs(position - node.center);
    return delta.x <= node.halfSize && delta.y <= node.halfSize && delta.z <= node.halfSize;
}
}

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
    double bodyMass          = body->getMass(BodyLock::NOLOCK);
    float childHalfSize     = node->halfSize * 0.5f;

    ThermalProperties props = body->getThermalProperties(BodyLock::NOLOCK);
    double area = body->getSurfaceArea();
    double epsArea = Physics::Thermal::effectiveEmissivity(props, props.tempK) * area;
    double emission = epsArea * Physics::Thermal::fourthPower(Physics::Thermal::clampTemperature(props.tempK));

    // Node is empty, put the body here
    if (node->body == nullptr && node->totalMass == 0.0) {
        node->body       = body;
        node->bodies.push_back(body);
        node->massCenter = bodyPos;
        node->totalMass  = bodyMass;
        node->totalEffectiveArea = epsArea;
        node->totalEmission = emission;
        return;
    }

    // Since this node now contains this body, update
    double newMass       = node->totalMass + bodyMass;
    node->massCenter += (bodyPos - node->massCenter) * static_cast<float>(bodyMass / newMass);
    node->totalMass     = newMass;
    node->totalEffectiveArea += epsArea;
    node->totalEmission += emission;

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

    Physics::PhysicsBody* existingBody = node->body;
    if (node->isLeaf() && !node->bodies.empty()) {
        if (childHalfSize <= kMinNodeHalfSize || (existingBody != nullptr && existingBody->getPosition(BodyLock::NOLOCK) == bodyPos)) {
            node->bodies.push_back(body);
            node->body = node->bodies.size() == 1 ? node->bodies.front() : nullptr;
            return;
        }

        std::vector<Physics::PhysicsBody*> existingBodies = std::move(node->bodies);
        node->bodies.clear();
        node->body = nullptr;
        for (Physics::PhysicsBody* existing : existingBodies) {
            insertBody(existing);
        }
        insertBody(body);
        return;
    }

    if (existingBody != nullptr) {
        std::vector<Physics::PhysicsBody*> existingBodies = std::move(node->bodies);
        node->bodies.clear();
        node->body = nullptr;
        for (Physics::PhysicsBody* existing : existingBodies) {
            insertBody(existing);
        }
    }
    insertBody(body);
}

void Octree::build(const std::vector<Physics::PhysicsBody*>& bodies) {
    Octree::clear();
    if (bodies.empty()) return;
    nodes.reserve(bodies.size() * 2);

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

glm::vec3 Octree::computeForce(Physics::PhysicsBody* body, double G) {
    if (nodes.empty() || body == nullptr) {
        return glm::vec3(0.0f);
    }

    glm::vec3 totalForce(0.0f);
    glm::vec3 bodyPos   = body->getPosition(BodyLock::NOLOCK);
    double bodyMass      = body->getMass(BodyLock::NOLOCK);

    std::vector<NodeIndex> stack;
    stack.reserve(kTraversalStackReserve);
    stack.push_back(NodeIndex::rootIndex());

    while (!stack.empty()) {
        NodeIndex currentIdx = stack.back();
        stack.pop_back();
        const OctreeNode& node = nodes[currentIdx.val];

        // Empty region
        if (node.totalMass == 0.0) continue;

        glm::vec3 dist = node.massCenter - body->getPosition(BodyLock::NOLOCK);
        float distSq = glm::dot(dist, dist);
        float softeningDistSq = distSq + Constants::SOFTENING_SQ;
        float widthSq = node.halfSize * node.halfSize * 4.0f;
        const bool nodeContainsBody = containsPosition(node, bodyPos);

        if (node.isLeaf() || (!nodeContainsBody && widthSq < Constants::THETA_SQ * distSq)) {
            if (node.isLeaf() && !node.bodies.empty()) {
                for (Physics::PhysicsBody* other : node.bodies) {
                    if (other == body) continue;
                    glm::vec3 pairDist = other->getPosition(BodyLock::NOLOCK) - bodyPos;
                    float pairDistSq = glm::dot(pairDist, pairDist) + Constants::SOFTENING_SQ;
                    float invPairDist = 1.0f / std::sqrt(pairDistSq);
                    float invPairDist3 = invPairDist * invPairDist * invPairDist;
                    double pairForce = (G * bodyMass * other->getMass(BodyLock::NOLOCK)) * invPairDist3;
                    totalForce += static_cast<float>(pairForce) * pairDist;
                }
                continue;
            }

            if (node.body == body) continue;

            float invDist = 1.0f / sqrt(softeningDistSq);
            float invDist3 = invDist * invDist * invDist;

            double force = (G * bodyMass * node.totalMass) * invDist3;
            totalForce += static_cast<float>(force) * dist;
        } else {
            // Add valid children to stack
            std::uint8_t childMask = node.childMask;
            while (childMask) {
                int childOctant = std::countr_zero(childMask);
                stack.push_back(node.children[childOctant]);
                childMask &= (childMask - 1);
            }
        }
    }
    return totalForce;
}

double Octree::computeHeat(Physics::PhysicsBody* body) {
    if (nodes.empty() || body == nullptr) {
        return 0.0;
    }

    double totalHeat = 0.0;
    glm::vec3 bodyPos = body->getPosition(BodyLock::NOLOCK);
    ThermalProperties props = body->getThermalProperties(BodyLock::NOLOCK);
    const double absorptivity = Physics::Thermal::effectiveAbsorptivity(props, props.tempK);
    double area = body->getSurfaceArea();
    double projectedArea = area * 0.25;

    std::vector<NodeIndex> stack;
    stack.reserve(kTraversalStackReserve);
    stack.push_back(NodeIndex::rootIndex());

    while (!stack.empty()) {
        NodeIndex currentIdx = stack.back();
        stack.pop_back();
        const OctreeNode& node = nodes[currentIdx.val];

        if (node.totalEffectiveArea == 0.0) continue;

        glm::vec3 dist = node.massCenter - bodyPos;
        double distSq = static_cast<double>(glm::dot(dist, dist));
        if (distSq < kMinRadiationDistanceSq) distSq = kMinRadiationDistanceSq;

        float widthSq = node.halfSize * node.halfSize * 4.0f;
        const bool nodeContainsBody = containsPosition(node, bodyPos);

        if (node.isLeaf()) {
            for (Physics::PhysicsBody* other : node.bodies) {
                if (other == body) continue;
                glm::vec3 pairDist = other->getPosition(BodyLock::NOLOCK) - bodyPos;
                double pairDistSq = static_cast<double>(glm::dot(pairDist, pairDist));
                if (pairDistSq < kMinRadiationDistanceSq) pairDistSq = kMinRadiationDistanceSq;

                double otherArea = other->getSurfaceArea();
                ThermalProperties otherProps = other->getThermalProperties(BodyLock::NOLOCK);
                double other_t_4 = Physics::Thermal::fourthPower(Physics::Thermal::clampTemperature(otherProps.tempK));
                double viewFactorTerm = (projectedArea * otherArea) / (4.0 * glm::pi<double>() * pairDistSq);
                viewFactorTerm = std::min(viewFactorTerm, projectedArea);
                double q_rad = Constants::STEFAN_BOLTZMANN * absorptivity * Physics::Thermal::effectiveEmissivity(otherProps, otherProps.tempK) * viewFactorTerm * other_t_4;
                totalHeat += q_rad;
            }

        } else if (!nodeContainsBody && widthSq < Constants::THETA_SQ * distSq) {
            double solidAngleFactor = projectedArea / (4.0 * glm::pi<double>() * distSq);
            double q_rad = Constants::STEFAN_BOLTZMANN * absorptivity * solidAngleFactor * node.totalEmission;
            totalHeat += q_rad;

        } else {
            std::uint8_t childMask = node.childMask;
            while (childMask) {
                int childOctant = std::countr_zero(childMask);
                stack.push_back(node.children[childOctant]);
                childMask &= (childMask - 1);
            }
        }
    }
    return totalHeat;
}
