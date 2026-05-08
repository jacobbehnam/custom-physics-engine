#pragma once

#include "TriangleBvh.h"
#include "graphics/components/ComputeShader.h"
#include "physics/PhysicsBody.h"

#include <glm/vec3.hpp>
#include <memory>
#include <optional>
#include <vector>
#include <cstdint>

class Camera;
class SceneManager;
class QOpenGLFunctions_4_5_Core;

// Progressive GPU ray traced view. Compute shader support is required.
class SceneRayTracer {
public:
    SceneRayTracer(SceneManager* sm, QOpenGLFunctions_4_5_Core* gl);
    ~SceneRayTracer();

    void setEnabled(bool e) { m_enabled = e; }
    /// fbWidth/Height = window pixels; internalScale in [0.25,1] traces at lower res then upscales (large perf win).
    void render(int fbWidth, int fbHeight, Camera* camera, const std::optional<std::vector<ObjectSnapshot>>& snapshots,
        float internalScale);

    bool isUsable() const { return m_presentOk && m_computeOk; }

private:
    std::unique_ptr<ComputeShader> m_shader;

    void ensureOutputSize(int w, int h);
    void ensureSSBOs(size_t nTri, size_t nNode);
    void uploadLights();
    void buildAndUpload();
    void refitAndUpload();
    void gather(std::vector<Raytrace::WorldTriangle>& out);
    size_t quickTriCount() const;
    uint64_t structureHash() const;
    uint64_t geometryHash() const;
    uint64_t viewHash(int w, int h, float internalScale, const Camera* camera) const;
    void maybeRebuildAccel();
    void resetAccumulation();
    void renderGpu(int w, int h, const Camera* camera);
    void presentOutput(int fbWidth, int fbHeight);

    SceneManager* m_sm{nullptr};
    QOpenGLFunctions_4_5_Core* m_g{nullptr};

    bool m_enabled{true};
    bool m_computeOk{false};
    bool m_presentOk{false};
    bool m_computeAttempted{false};
    bool m_presentAttempted{false};

    unsigned m_present{0};

    int m_fbw{0};
    int m_fbh{0};

    unsigned m_outTex{0};

    Raytrace::TriangleBvh m_bvh;
    size_t m_triCap{0};
    size_t m_nodeCap{0};
    size_t m_lightCap{0};
    size_t m_lightCount{0};
    glm::vec3 m_renderOrigin{0.0f};
    unsigned m_triSsb{0};
    unsigned m_nodeSsb{0};
    unsigned m_lightSsb{0};

    unsigned m_fsqVao{0};

    uint64_t m_lastGeomHash{0};
    uint64_t m_lastStructureHash{0};
    size_t m_lastTriCount{static_cast<size_t>(-1)};
    uint64_t m_lastViewHash{0};
    uint32_t m_accumulatedFrames{0};
    bool m_lastEnableGlobalLight{true};

    void ensureComputeProgram();
    void ensurePresentProgram();
};
