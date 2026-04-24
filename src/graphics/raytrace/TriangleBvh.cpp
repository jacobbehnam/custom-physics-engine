#include "TriangleBvh.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <limits>
#include <numeric>

#include <future>
#include <thread>

namespace Raytrace {

int TriangleBvh::allocNode() {
    return m_nodeCount.fetch_add(1, std::memory_order_relaxed);
}

void TriangleBvh::mergeAabb(
    const glm::vec3& a0, const glm::vec3& a1, const glm::vec3& b0, const glm::vec3& b1, glm::vec3& o0, glm::vec3& o1) const
{
    o0 = glm::min(a0, b0);
    o1 = glm::max(a1, b1);
}

void TriangleBvh::boundsByOrder(const std::vector<WorldTriangle>& t, const std::vector<int>& order, int oLo, int oHi,
    glm::vec3& bmin, glm::vec3& bmax) const
{
    bmin = glm::vec3(std::numeric_limits<float>::max());
    bmax = glm::vec3(-std::numeric_limits<float>::max());
    for (int oi = oLo; oi < oHi; ++oi) {
        const auto& a = t[static_cast<size_t>(order[static_cast<size_t>(oi)])];
        bmin = glm::min(bmin, glm::min(a.p0, glm::min(a.p1, a.p2)));
        bmax = glm::max(bmax, glm::max(a.p0, glm::max(a.p1, a.p2)));
    }
}

int TriangleBvh::longestAxis(const glm::vec3& e) const
{
    if (e.x >= e.y && e.x >= e.z) {
        return 0;
    }
    if (e.y >= e.z) {
        return 1;
    }
    return 2;
}

glm::vec3 TriangleBvh::centroidOf(const WorldTriangle& a) {
    return (a.p0 + a.p1 + a.p2) * (1.0f / 3.0f);
}

void TriangleBvh::build(const std::vector<WorldTriangle>& in) {
    if (in.empty()) {
        m_nodes.clear();
        m_gpu.clear();
        m_root = 0;
        return;
    }
    if (m_nodes.capacity() < 2u * in.size() + 2u) {
        m_nodes.reserve(2u * in.size() + 2u);
    }
    m_nodes.resize(2u * in.size() + 2u);
    m_nodeCount = 0;
    
    thread_local std::vector<int> order;
    thread_local std::vector<glm::vec3> centroids;
    if (order.capacity() < in.size()) order.reserve(in.size());
    if (centroids.capacity() < in.size()) centroids.reserve(in.size());
    order.resize(in.size());
    centroids.resize(in.size());
    std::iota(order.begin(), order.end(), 0);
    for(size_t i = 0; i < in.size(); ++i) {
        centroids[i] = centroidOf(in[i]);
    }

    m_root = buildRange(in, centroids, order, 0, static_cast<int>(order.size()), 0);
    
    // Shrink to actual used size
    m_nodes.resize(m_nodeCount.load());

    m_gpu.resize(in.size());
    for(size_t i = 0; i < in.size(); ++i) {
        const WorldTriangle& t = in[order[i]];
        m_gpu[i] = {glm::vec4(t.p0, 0.0f),
                    glm::vec4(t.p1, 0.0f),
                    glm::vec4(t.p2, 0.0f),
                    glm::vec4(t.s0, 0.0f),
                    glm::vec4(t.s1, 0.0f),
                    glm::vec4(t.s2, 0.0f),
                    glm::vec4(t.albedo, static_cast<float>(t.materialKind))};
    }
}

uint32_t TriangleBvh::buildRange(const std::vector<WorldTriangle>& tris, const std::vector<glm::vec3>& centroids, std::vector<int>& order, int oLo, int oHi, int depth) {
    glm::vec3 bmin, bmax;
    boundsByOrder(tris, order, oLo, oHi, bmin, bmax);

    if (oHi - oLo <= 4) {
        int n = allocNode();
        uint32_t count = static_cast<uint32_t>(oHi - oLo);
        uint32_t start = static_cast<uint32_t>(oLo);
        m_nodes[static_cast<size_t>(n)] = GpuBvhNode{bmin, kLeaf | (count << 28) | start, bmax, 0u};
        return static_cast<uint32_t>(n);
    }

    glm::vec3 ext = bmax - bmin;
    int ax = longestAxis(ext);
    
    // SAH parameters
    const int BINS = 16;
    float boundsMin = bmin[ax];
    float boundsMax = bmax[ax];
    
    if (boundsMax - boundsMin < 1e-5f) {
        int mid = (oLo + oHi) / 2;
        uint32_t l = buildRange(tris, centroids, order, oLo, mid, depth + 1);
        uint32_t r = buildRange(tris, centroids, order, mid, oHi, depth + 1);
        int p = allocNode();
        m_nodes[static_cast<size_t>(p)] = GpuBvhNode{bmin, l, bmax, r};
        return static_cast<uint32_t>(p);
    }

    struct Bin {
        int count = 0;
        glm::vec3 bmin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 bmax = glm::vec3(-std::numeric_limits<float>::max());
    } bins[BINS];

    float scale = BINS / (boundsMax - boundsMin);
    for (int i = oLo; i < oHi; ++i) {
        int idx = order[i];
        int b = static_cast<int>((centroids[idx][ax] - boundsMin) * scale);
        if (b < 0) b = 0;
        if (b >= BINS) b = BINS - 1;
        bins[b].count++;
        const auto& t = tris[idx];
        bins[b].bmin = glm::min(bins[b].bmin, glm::min(t.p0, glm::min(t.p1, t.p2)));
        bins[b].bmax = glm::max(bins[b].bmax, glm::max(t.p0, glm::max(t.p1, t.p2)));
    }

    float leftArea[BINS - 1], rightArea[BINS - 1];
    int leftCount[BINS - 1], rightCount[BINS - 1];
    
    glm::vec3 lmin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 lmax = glm::vec3(-std::numeric_limits<float>::max());
    int sumCount = 0;
    for (int i = 0; i < BINS - 1; ++i) {
        sumCount += bins[i].count;
        leftCount[i] = sumCount;
        if (bins[i].count > 0) {
            lmin = glm::min(lmin, bins[i].bmin);
            lmax = glm::max(lmax, bins[i].bmax);
        }
        glm::vec3 e = lmax - lmin;
        leftArea[i] = e.x*e.y + e.y*e.z + e.z*e.x;
    }
    
    glm::vec3 rmin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 rmax = glm::vec3(-std::numeric_limits<float>::max());
    sumCount = 0;
    for (int i = BINS - 1; i > 0; --i) {
        sumCount += bins[i].count;
        rightCount[i - 1] = sumCount;
        if (bins[i].count > 0) {
            rmin = glm::min(rmin, bins[i].bmin);
            rmax = glm::max(rmax, bins[i].bmax);
        }
        glm::vec3 e = rmax - rmin;
        rightArea[i - 1] = e.x*e.y + e.y*e.z + e.z*e.x;
    }

    float minCost = std::numeric_limits<float>::max();
    int bestSplit = -1;
    for (int i = 0; i < BINS - 1; ++i) {
        if (leftCount[i] == 0 || rightCount[i] == 0) continue;
        float cost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
        if (cost < minCost) {
            minCost = cost;
            bestSplit = i;
        }
    }

    int mid = oLo;
    if (bestSplit != -1) {
        auto it = std::partition(order.begin() + oLo, order.begin() + oHi, [&](int idx) {
            int b = static_cast<int>((centroids[idx][ax] - boundsMin) * scale);
            if (b < 0) b = 0;
            if (b >= BINS) b = BINS - 1;
            return b <= bestSplit;
        });
        mid = static_cast<int>(it - order.begin());
    }

    if (mid == oLo || mid == oHi) {
        mid = (oLo + oHi) / 2;
        std::nth_element(order.begin() + oLo, order.begin() + mid, order.begin() + oHi, [&](int ia, int ib) {
            return centroids[ia][ax] < centroids[ib][ax];
        });
    }

    uint32_t l, r;
    if (depth < 4 && (oHi - oLo) > 1000) {
        auto futureL = std::async(std::launch::async, [&]() { return buildRange(tris, centroids, order, oLo, mid, depth + 1); });
        r = buildRange(tris, centroids, order, mid, oHi, depth + 1);
        l = futureL.get();
    } else {
        l = buildRange(tris, centroids, order, oLo, mid, depth + 1);
        r = buildRange(tris, centroids, order, mid, oHi, depth + 1);
    }
    
    glm::vec3 mb0, mb1;
    mergeAabb(m_nodes[static_cast<size_t>(l)].bmin, m_nodes[static_cast<size_t>(l)].bmax, 
              m_nodes[static_cast<size_t>(r)].bmin, m_nodes[static_cast<size_t>(r)].bmax, mb0, mb1);
    
    int p = allocNode();
    m_nodes[static_cast<size_t>(p)] = GpuBvhNode{mb0, l, mb1, r};
    return static_cast<uint32_t>(p);
}

} // namespace Raytrace
