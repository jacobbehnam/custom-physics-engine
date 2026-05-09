#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>

namespace Raytrace {

enum class MaterialKind : uint32_t {
    Matte = 0u,
    Checkerboard = 1u,
    Sphere = 2u,
};

struct alignas(16) GpuBvhNode {
    glm::vec3 bmin{};
    uint32_t c0{0};
    glm::vec3 bmax{};
    uint32_t c1{0};
};

struct alignas(16) GpuTri {
    glm::vec4 v0{};
    glm::vec4 v1{};
    glm::vec4 v2{};
    glm::vec4 n0{};
    glm::vec4 n1{};
    glm::vec4 n2{};
    glm::vec4 material{};
    glm::vec4 emissive{};
};

struct alignas(16) GpuLight {
    glm::vec4 positionRadius{};
    glm::vec4 emissive{};
};

struct alignas(16) GpuSphere {
    glm::vec4 centerRadius{};
    glm::vec4 material{};
    glm::vec4 emissive{};
};

struct WorldTriangle {
    glm::vec3 p0, p1, p2;
    glm::vec3 s0, s1, s2;
    glm::vec3 sphereCenter{0.0f};
    glm::vec3 albedo{0.0f};
    MaterialKind materialKind{MaterialKind::Matte};
    glm::vec3 emissive{0.0f};
    uint32_t objectID{0};
};

class TriangleBvh {
public:
    void build(const std::vector<WorldTriangle>& in);
    void refit(const std::vector<WorldTriangle>& in);
    bool canRefit(size_t triCount) const { return !m_nodes.empty() && !m_order.empty() && m_order.size() == triCount; }

    const std::vector<GpuBvhNode>& getNodes() const { return m_nodes; }
    const std::vector<GpuTri>& getGpuTris() const { return m_gpu; }
    uint32_t getRoot() const { return m_root; }
    bool isEmpty() const { return m_gpu.empty(); }

private:
    static constexpr int      kMaxLeafTris = 7;
    struct RangeBounds {
        glm::vec3 bmin{};
        glm::vec3 bmax{};
        glm::vec3 cmin{};
        glm::vec3 cmax{};
    };

    int m_nodeCount{0};
    int allocNode();
    RangeBounds computeRangeBounds(
        const std::vector<WorldTriangle>& tris, const std::vector<glm::vec3>& centroids, const std::vector<int>& order,
        int oLo, int oHi) const;
    int longestAxis(const glm::vec3& e) const;
    void refitNodes();
    uint32_t buildRange(
        const std::vector<WorldTriangle>& tris, const std::vector<glm::vec3>& centroids, std::vector<int>& order,
        int oLo, int oHi);
    static GpuTri packTriangle(const WorldTriangle& tri);

    std::vector<GpuBvhNode> m_nodes;
    std::vector<GpuTri> m_gpu;
    std::vector<int> m_order;
    uint32_t m_root{0};

    static glm::vec3 centroidOf(const WorldTriangle& a);
};

} // namespace Raytrace
