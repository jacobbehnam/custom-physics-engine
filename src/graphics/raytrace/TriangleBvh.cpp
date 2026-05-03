#include "TriangleBvh.h"

#include <algorithm>
#include <glm/glm.hpp>
#include <limits>
#include <numeric>

namespace Raytrace {

namespace {
constexpr uint32_t kLeaf           = 0x80000000u;
constexpr uint32_t kLeafStartMask  = 0x0FFFFFFFu;
constexpr uint32_t kLeafCountShift = 28u;
constexpr uint32_t kLeafCountMask  = 0x7u;

uint32_t decodeLeafCount(uint32_t packed) {
    return (packed >> kLeafCountShift) & kLeafCountMask;
}

uint32_t decodeLeafStart(uint32_t packed) {
    return packed & kLeafStartMask;
}
} // namespace

int TriangleBvh::allocNode() {
    return m_nodeCount++;
}

TriangleBvh::RangeBounds TriangleBvh::computeRangeBounds(
    const std::vector<WorldTriangle>& tris,
    const std::vector<glm::vec3>& centroids,
    const std::vector<int>& order,
    int oLo,
    int oHi) const {
    RangeBounds bounds;
    bounds.bmin = glm::vec3(std::numeric_limits<float>::max());
    bounds.bmax = glm::vec3(-std::numeric_limits<float>::max());
    bounds.cmin = glm::vec3(std::numeric_limits<float>::max());
    bounds.cmax = glm::vec3(-std::numeric_limits<float>::max());

    for (int oi = oLo; oi < oHi; ++oi) {
        const int triIndex = order[static_cast<size_t>(oi)];
        const WorldTriangle& tri = tris[static_cast<size_t>(triIndex)];
        const glm::vec3 triMin = glm::min(tri.p0, glm::min(tri.p1, tri.p2));
        const glm::vec3 triMax = glm::max(tri.p0, glm::max(tri.p1, tri.p2));
        bounds.bmin = glm::min(bounds.bmin, triMin);
        bounds.bmax = glm::max(bounds.bmax, triMax);
        bounds.cmin = glm::min(bounds.cmin, centroids[static_cast<size_t>(triIndex)]);
        bounds.cmax = glm::max(bounds.cmax, centroids[static_cast<size_t>(triIndex)]);
    }

    return bounds;
}

int TriangleBvh::longestAxis(const glm::vec3& e) const {
    if (e.x >= e.y && e.x >= e.z) {
        return 0;
    }
    if (e.y >= e.z) {
        return 1;
    }
    return 2;
}

glm::vec3 TriangleBvh::centroidOf(const WorldTriangle& tri) {
    return (tri.p0 + tri.p1 + tri.p2) * (1.0f / 3.0f);
}

GpuTri TriangleBvh::packTriangle(const WorldTriangle& tri) {
    return {
        glm::vec4(tri.p0, 0.0f),
        glm::vec4(tri.p1, 0.0f),
        glm::vec4(tri.p2, 0.0f),
        glm::vec4(tri.s0, 0.0f),
        glm::vec4(tri.s1, 0.0f),
        glm::vec4(tri.s2, 0.0f),
        glm::vec4(tri.albedo, static_cast<float>(tri.materialKind)),
        glm::vec4(tri.emissive, 0.0f),
    };
}

void TriangleBvh::build(const std::vector<WorldTriangle>& in) {
    if (in.empty()) {
        m_nodes.clear();
        m_gpu.clear();
        m_order.clear();
        m_root = 0;
        m_nodeCount = 0;
        return;
    }

    m_nodes.resize(2u * in.size() + 2u);
    m_nodeCount = 0;

    std::vector<int> order(in.size());
    std::vector<glm::vec3> centroids(in.size());
    std::iota(order.begin(), order.end(), 0);
    for (size_t i = 0; i < in.size(); ++i) {
        centroids[i] = centroidOf(in[i]);
    }

    m_root = buildRange(in, centroids, order, 0, static_cast<int>(order.size()));
    m_nodes.resize(static_cast<size_t>(m_nodeCount));
    m_order = order;

    m_gpu.resize(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        m_gpu[i] = packTriangle(in[static_cast<size_t>(m_order[i])]);
    }
}

void TriangleBvh::refit(const std::vector<WorldTriangle>& in) {
    if (!canRefit(in.size())) {
        build(in);
        return;
    }

    for (size_t i = 0; i < in.size(); ++i) {
        m_gpu[i] = packTriangle(in[static_cast<size_t>(m_order[i])]);
    }
    refitNodes();
}

void TriangleBvh::refitNodes() {
    for (GpuBvhNode& node : m_nodes) {
        if ((node.c0 & kLeaf) != 0u) {
            const uint32_t start = decodeLeafStart(node.c0);
            const uint32_t count = decodeLeafCount(node.c0);
            glm::vec3 bmin(std::numeric_limits<float>::max());
            glm::vec3 bmax(-std::numeric_limits<float>::max());

            for (uint32_t i = 0; i < count; ++i) {
                const GpuTri& tri = m_gpu[static_cast<size_t>(start + i)];
                const glm::vec3 p0(tri.v0);
                const glm::vec3 p1(tri.v1);
                const glm::vec3 p2(tri.v2);
                bmin = glm::min(bmin, glm::min(p0, glm::min(p1, p2)));
                bmax = glm::max(bmax, glm::max(p0, glm::max(p1, p2)));
            }
            node.bmin = bmin;
            node.bmax = bmax;
            continue;
        }

        const GpuBvhNode& left = m_nodes[static_cast<size_t>(node.c0)];
        const GpuBvhNode& right = m_nodes[static_cast<size_t>(node.c1)];
        node.bmin = glm::min(left.bmin, right.bmin);
        node.bmax = glm::max(left.bmax, right.bmax);
    }
}

uint32_t TriangleBvh::buildRange(
    const std::vector<WorldTriangle>& tris,
    const std::vector<glm::vec3>& centroids,
    std::vector<int>& order,
    int oLo,
    int oHi) {
    const RangeBounds bounds = computeRangeBounds(tris, centroids, order, oLo, oHi);
    const int count = oHi - oLo;

    if (count <= kMaxLeafTris) {
        const int nodeIndex = allocNode();
        m_nodes[static_cast<size_t>(nodeIndex)] = GpuBvhNode{
            bounds.bmin,
            kLeaf | (static_cast<uint32_t>(count) << kLeafCountShift) | static_cast<uint32_t>(oLo),
            bounds.bmax,
            0u,
        };
        return static_cast<uint32_t>(nodeIndex);
    }

    const int axis = longestAxis(bounds.cmax - bounds.cmin);
    int mid = (oLo + oHi) / 2;
    std::nth_element(
        order.begin() + oLo,
        order.begin() + mid,
        order.begin() + oHi,
        [&](int lhs, int rhs) { return centroids[static_cast<size_t>(lhs)][axis] < centroids[static_cast<size_t>(rhs)][axis]; });

    if (mid == oLo || mid == oHi) {
        mid = oLo + count / 2;
    }

    const uint32_t left = buildRange(tris, centroids, order, oLo, mid);
    const uint32_t right = buildRange(tris, centroids, order, mid, oHi);

    const int nodeIndex = allocNode();
    m_nodes[static_cast<size_t>(nodeIndex)] = GpuBvhNode{
        glm::min(m_nodes[static_cast<size_t>(left)].bmin, m_nodes[static_cast<size_t>(right)].bmin),
        left,
        glm::max(m_nodes[static_cast<size_t>(left)].bmax, m_nodes[static_cast<size_t>(right)].bmax),
        right,
    };
    return static_cast<uint32_t>(nodeIndex);
}

} // namespace Raytrace
