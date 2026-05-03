#include "SceneRayTracer.h"

#include "ui/AppSettings.h"
#include "ui/settings/GraphicsSettings.h"
#include "graphics/components/Mesh.h"
#include "graphics/core/Camera.h"
#include "graphics/core/ResourceManager.h"
#include "graphics/core/SceneManager.h"
#include "graphics/core/SceneObject.h"

#include <QByteArray>
#include <QOpenGLContext>
#include <QOpenGLFunctions_4_5_Core>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <span>

namespace {

using Raytrace::GpuBvhNode;
using Raytrace::GpuLight;
using Raytrace::GpuTri;
using Raytrace::MaterialKind;

constexpr float    kFarClip                = 300000.0f;
constexpr float    kGpuInteractiveMaxScale = 0.6f;
constexpr float    kEmptySceneClearRed     = 0.40f;
constexpr float    kEmptySceneClearGreen   = 0.52f;
constexpr float    kEmptySceneClearBlue    = 0.66f;
constexpr float    kLightPowerEpsilon      = 1.0e-5f;
constexpr float    kMinAnalyticLightRadius = 0.1f;
constexpr size_t   kMaxAnalyticLights      = 8;
constexpr uint32_t kShaderAaPatternCount   = 4;
constexpr uint32_t kTemporalSampleCount    = 1 + kShaderAaPatternCount;

static GLuint compileShader(GLenum type, const std::string& source, const char* label, QOpenGLFunctions_4_5_Core* gl) {
    const char* src = source.c_str();
    const GLuint shader = gl->glCreateShader(type);
    gl->glShaderSource(shader, 1, &src, nullptr);
    gl->glCompileShader(shader);

    GLint ok = 0;
    gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        char log[2048];
        gl->glGetShaderInfoLog(shader, sizeof(log) - 1, nullptr, log);
        std::cerr << "[raytrace] compile failure in " << label << '\n' << log << std::endl;
        gl->glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static uint64_t fnvAppend(uint64_t h, uint32_t bits) {
    h ^= static_cast<uint64_t>(bits);
    h *= 1099511628211ull;
    return h;
}

static uint64_t fnvAppendFloat(uint64_t h, float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    return fnvAppend(h, bits);
}

static float hash01(uint32_t v) {
    v ^= v >> 16u;
    v *= 0x7feb352du;
    v ^= v >> 15u;
    v *= 0x846ca68bu;
    v ^= v >> 16u;
    return static_cast<float>(v & 0x00ffffffu) * (1.0f / 16777215.0f);
}

static float luminance(const glm::vec3& c) {
    return glm::dot(c, glm::vec3(0.2126f, 0.7152f, 0.0722f));
}

static float lightImportance(const GpuLight& light) {
    const float radius = light.positionRadius.w;
    return luminance(glm::vec3(light.emissive)) * radius * radius;
}

static glm::vec3 pickObjectAlbedo(const SceneObject& object) {
    if (object.getShader() == ResourceManager::getShader("checkerboard")) {
        return glm::vec3(0.22f);
    }

    glm::vec3 base(0.86f, 0.78f, 0.30f);
    const float tint = hash01(object.getObjectID() * 9781u + 0x9e3779b9u) - 0.5f;
    base += glm::vec3(0.10f * tint, 0.06f * tint, -0.04f * tint);
    return glm::clamp(base, glm::vec3(0.15f), glm::vec3(0.98f));
}

static MaterialKind pickMaterialKind(const SceneObject& object) {
    return object.getShader() == ResourceManager::getShader("checkerboard")
        ? MaterialKind::Checkerboard
        : MaterialKind::Matte;
}

static bool isFiniteVec3(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

} // namespace

void SceneRayTracer::ensureComputeProgram() {
    if (m_computeAttempted || m_shader) {
        return;
    }
    m_computeAttempted = true;

    auto* ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        std::cerr << "[raytrace] no current OpenGL context" << std::endl;
        return;
    }

    const auto fmt = ctx->format();
    const bool supportsCompute =
        (fmt.majorVersion() > 4 || (fmt.majorVersion() == 4 && fmt.minorVersion() >= 3))
        || ctx->hasExtension(QByteArrayLiteral("GL_ARB_compute_shader"));

    if (!supportsCompute) {
        std::cerr << "[raytrace] compute shaders unavailable; ray traced view disabled." << std::endl;
        return;
    }

    const std::string shaderPath = "assets/shaders/raytrace/raytrace.comp";

    m_shader = std::make_unique<ComputeShader>(shaderPath, m_g);

    if (m_shader->id() == 0) {
        std::cerr << "[raytrace] compute program failed to build." << std::endl;
        m_shader.reset();
        return;
    }

    m_computeOk = true;
}

void SceneRayTracer::ensurePresentProgram() {
    if (m_presentAttempted || m_present) {
        return;
    }
    m_presentAttempted = true;

    const std::string vs = R"(#version 430 core
out vec2 vTex;
const vec2 pos[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
const vec2 uv[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
void main() {
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
    vTex = uv[gl_VertexID];
}
)";

    const std::string fs = R"(#version 430 core
in vec2 vTex;
out vec4 o;
layout(binding = 0) uniform sampler2D t;

const float DENOISE_CENTER_WEIGHT   = 4.0;
const float DENOISE_LUMA_STRENGTH   = 6.0;
const float DENOISE_DIAGONAL_WEIGHT = 0.65;

float luminance(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec3 denoise(vec2 uv) {
    ivec2 size = textureSize(t, 0);
    vec2 texel = 1.0 / vec2(size);
    vec3 center = texture(t, uv).rgb;
    float centerLum = luminance(center);
    vec3 sum = center * DENOISE_CENTER_WEIGHT;
    float weightSum = DENOISE_CENTER_WEIGHT;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            if (x == 0 && y == 0) {
                continue;
            }
            vec3 sampleColor = texture(t, uv + vec2(x, y) * texel).rgb;
            float colorDelta = abs(luminance(sampleColor) - centerLum);
            float weight = exp(-colorDelta * DENOISE_LUMA_STRENGTH)
                * ((x == 0 || y == 0) ? 1.0 : DENOISE_DIAGONAL_WEIGHT);
            sum += sampleColor * weight;
            weightSum += weight;
        }
    }

    return sum / weightSum;
}

vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 col = denoise(vTex);
    col = aces(col);
    col = pow(col, vec3(1.0 / 2.2));
    o = vec4(col, 1.0);
}
)";

    const GLuint vert = compileShader(GL_VERTEX_SHADER, vs, "raytrace-present.vert", m_g);
    const GLuint frag = compileShader(GL_FRAGMENT_SHADER, fs, "raytrace-present.frag", m_g);
    if (!vert || !frag) {
        if (vert) {
            m_g->glDeleteShader(vert);
        }
        if (frag) {
            m_g->glDeleteShader(frag);
        }
        return;
    }

    m_present = m_g->glCreateProgram();
    m_g->glAttachShader(m_present, vert);
    m_g->glAttachShader(m_present, frag);
    m_g->glLinkProgram(m_present);
    m_g->glDeleteShader(vert);
    m_g->glDeleteShader(frag);

    GLint linked = 0;
    m_g->glGetProgramiv(m_present, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char log[2048];
        m_g->glGetProgramInfoLog(m_present, sizeof(log) - 1, nullptr, log);
        std::cerr << "[raytrace] present link failure\n" << log << std::endl;
        m_g->glDeleteProgram(m_present);
        m_present = 0;
        return;
    }

    m_g->glUseProgram(m_present);
    const GLint sampler = m_g->glGetUniformLocation(m_present, "t");
    if (sampler >= 0) {
        m_g->glUniform1i(sampler, 0);
    }
    m_g->glUseProgram(0);
    m_presentOk = true;
}

void SceneRayTracer::ensureOutputSize(int w, int h) {
    w = std::max(w, 1);
    h = std::max(h, 1);
    if (w == m_fbw && h == m_fbh && m_outTex != 0) {
        return;
    }

    m_fbw = w;
    m_fbh = h;
    if (m_outTex) {
        m_g->glDeleteTextures(1, &m_outTex);
        m_outTex = 0;
    }

    m_g->glGenTextures(1, &m_outTex);
    m_g->glBindTexture(GL_TEXTURE_2D, m_outTex);
    m_g->glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, w, h);
    m_g->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_g->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_g->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_g->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_g->glBindTexture(GL_TEXTURE_2D, 0);

    resetAccumulation();
}

void SceneRayTracer::ensureSSBOs(size_t nTri, size_t nNode) {
    if (m_triSsb == 0) {
        m_g->glGenBuffers(1, &m_triSsb);
    }
    if (m_nodeSsb == 0) {
        m_g->glGenBuffers(1, &m_nodeSsb);
    }
    if (nTri > m_triCap) {
        m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_triSsb);
        m_g->glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(nTri * sizeof(GpuTri)), nullptr, GL_DYNAMIC_DRAW);
        m_triCap = nTri;
    }
    if (nNode > m_nodeCap) {
        m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_nodeSsb);
        m_g->glBufferData(
            GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(nNode * sizeof(GpuBvhNode)), nullptr, GL_DYNAMIC_DRAW);
        m_nodeCap = nNode;
    }
    m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SceneRayTracer::uploadLights() {
    std::vector<GpuLight> lights;
    lights.reserve(m_sm->getObjects().size());

    for (const auto& owned : m_sm->getObjects()) {
        const SceneObject* object = owned.get();
        const Physics::PhysicsBody* body = object->getPhysicsBody();
        if (!body) {
            continue;
        }

        const glm::vec3 emission = body->getEmission(BodyLock::LOCK);
        if (luminance(emission) <= kLightPowerEpsilon) {
            continue;
        }

        const glm::mat4 model = object->getModelMatrix();
        const glm::vec3 position = glm::vec3(model[3]);
        const glm::vec3 scale = glm::abs(object->getScale());
        const float radius = std::max(kMinAnalyticLightRadius, 0.5f * std::max({scale.x, scale.y, scale.z}));

        lights.push_back({
            glm::vec4(position, radius),
            glm::vec4(emission, 0.0f),
        });
    }

    std::sort(lights.begin(), lights.end(), [](const GpuLight& lhs, const GpuLight& rhs) {
        return lightImportance(lhs) > lightImportance(rhs);
    });
    if (lights.size() > kMaxAnalyticLights) {
        lights.resize(kMaxAnalyticLights);
    }

    m_lightCount = lights.size();
    if (m_lightSsb == 0) {
        m_g->glGenBuffers(1, &m_lightSsb);
    }

    m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lightSsb);
    if (m_lightCount == 0) {
        const GpuLight dummy{};
        if (m_lightCap == 0) {
            m_g->glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GpuLight), &dummy, GL_DYNAMIC_DRAW);
            m_lightCap = 1;
        } else {
            m_g->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GpuLight), &dummy);
        }
        m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return;
    }

    if (m_lightCount > m_lightCap) {
        m_g->glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<GLsizeiptr>(m_lightCount * sizeof(GpuLight)),
            lights.data(),
            GL_DYNAMIC_DRAW);
        m_lightCap = m_lightCount;
    } else {
        m_g->glBufferSubData(
            GL_SHADER_STORAGE_BUFFER,
            0,
            static_cast<GLsizeiptr>(m_lightCount * sizeof(GpuLight)),
            lights.data());
    }
    m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SceneRayTracer::gather(std::vector<Raytrace::WorldTriangle>& out) {
    out.clear();
    const size_t triCount = quickTriCount();
    if (out.capacity() < triCount) {
        out.reserve(triCount);
    }

    for (const auto& owned : m_sm->getObjects()) {
        const SceneObject* object = owned.get();
        Mesh* mesh = object->getMesh();
        if (!mesh) {
            continue;
        }

        const glm::mat4 model = object->getModelMatrix();
        const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        const glm::vec3 albedo = pickObjectAlbedo(*object);
        const MaterialKind materialKind = pickMaterialKind(*object);
        const std::span<const Vertex> vertices = mesh->getVertices();
        const std::span<const unsigned int> indices = mesh->getIndices();

        out.reserve(out.size() + indices.size() / 3u);
        for (size_t i = 0; i + 2u < indices.size(); i += 3u) {
            const unsigned int ia = indices[i];
            const unsigned int ib = indices[i + 1u];
            const unsigned int ic = indices[i + 2u];
            if (ia >= vertices.size() || ib >= vertices.size() || ic >= vertices.size()) {
                continue;
            }

            const Vertex& a = vertices[ia];
            const Vertex& b = vertices[ib];
            const Vertex& c = vertices[ic];

            const glm::vec3 w0 = glm::vec3(model * glm::vec4(a.pos, 1.0f));
            const glm::vec3 w1 = glm::vec3(model * glm::vec4(b.pos, 1.0f));
            const glm::vec3 w2 = glm::vec3(model * glm::vec4(c.pos, 1.0f));
            glm::vec3 n0 = normalMatrix * a.normal;
            glm::vec3 n1 = normalMatrix * b.normal;
            glm::vec3 n2 = normalMatrix * c.normal;
            const glm::vec3 faceN = glm::normalize(glm::cross(w1 - w0, w2 - w0));
            if (!isFiniteVec3(n0) || glm::dot(n0, n0) < 1.0e-8f) {
                n0 = faceN;
            }
            if (!isFiniteVec3(n1) || glm::dot(n1, n1) < 1.0e-8f) {
                n1 = faceN;
            }
            if (!isFiniteVec3(n2) || glm::dot(n2, n2) < 1.0e-8f) {
                n2 = faceN;
            }

            glm::vec3 emission{0.0f};
            if (const Physics::PhysicsBody* body = object->getPhysicsBody()) {
                emission = body->getEmission(BodyLock::LOCK);
            }

            out.push_back({
                w0,
                w1,
                w2,
                glm::normalize(n0),
                glm::normalize(n1),
                glm::normalize(n2),
                albedo,
                materialKind,
                emission,
            });
        }
    }
}

size_t SceneRayTracer::quickTriCount() const {
    size_t total = 0;
    for (const auto& owned : m_sm->getObjects()) {
        const Mesh* mesh = owned->getMesh();
        if (!mesh) {
            continue;
        }
        total += mesh->getIndices().size() / 3u;
    }
    return total;
}

uint64_t SceneRayTracer::structureHash() const {
    uint64_t h = 14695981039346656037ull;
    for (const auto& owned : m_sm->getObjects()) {
        const SceneObject* object = owned.get();
        const Mesh* mesh = object->getMesh();
        const Shader* shader = object->getShader();
        const uintptr_t meshPtr = reinterpret_cast<uintptr_t>(mesh);
        const uintptr_t shaderPtr = reinterpret_cast<uintptr_t>(shader);
        h = fnvAppend(h, static_cast<uint32_t>(meshPtr));
        h = fnvAppend(h, static_cast<uint32_t>(shaderPtr));
        if constexpr (sizeof(uintptr_t) > sizeof(uint32_t)) {
            h = fnvAppend(h, static_cast<uint32_t>(meshPtr >> 32u));
            h = fnvAppend(h, static_cast<uint32_t>(shaderPtr >> 32u));
        }
        h = fnvAppend(h, object->getObjectID());
        if (mesh) {
            h = fnvAppend(h, static_cast<uint32_t>(mesh->getIndices().size()));
            h = fnvAppend(h, static_cast<uint32_t>(mesh->getVertices().size()));
        }
    }
    return h;
}

uint64_t SceneRayTracer::geometryHash() const {
    uint64_t h = 14695981039346656037ull;
    for (const auto& owned : m_sm->getObjects()) {
        const SceneObject* object = owned.get();
        h = fnvAppend(h, object->getObjectID());

        const glm::mat4 model = object->getModelMatrix();
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                h = fnvAppendFloat(h, model[c][r]);
            }
        }
        if (const Physics::PhysicsBody* body = object->getPhysicsBody()) {
            const glm::vec3 emission = body->getEmission(BodyLock::LOCK);
            h = fnvAppendFloat(h, emission.x);
            h = fnvAppendFloat(h, emission.y);
            h = fnvAppendFloat(h, emission.z);
        }
    }
    return h;
}

uint64_t SceneRayTracer::viewHash(int w, int h, float internalScale, const Camera* camera) const {
    uint64_t h64 = 14695981039346656037ull;
    h64 = fnvAppend(h64, static_cast<uint32_t>(w));
    h64 = fnvAppend(h64, static_cast<uint32_t>(h));
    h64 = fnvAppendFloat(h64, internalScale);
    h64 = fnvAppendFloat(h64, camera->position.x);
    h64 = fnvAppendFloat(h64, camera->position.y);
    h64 = fnvAppendFloat(h64, camera->position.z);
    h64 = fnvAppendFloat(h64, camera->front.x);
    h64 = fnvAppendFloat(h64, camera->front.y);
    h64 = fnvAppendFloat(h64, camera->front.z);
    h64 = fnvAppendFloat(h64, camera->up.x);
    h64 = fnvAppendFloat(h64, camera->up.y);
    h64 = fnvAppendFloat(h64, camera->up.z);
    h64 = fnvAppendFloat(h64, camera->fov);
    return h64;
}

void SceneRayTracer::resetAccumulation() {
    m_accumulatedFrames = 0;
    m_lastViewHash = 0;
}

void SceneRayTracer::maybeRebuildAccel() {
    const size_t triCount = quickTriCount();
    if (triCount == 0) {
        if (m_lastTriCount != 0 || m_lastGeomHash != 0 || m_lastStructureHash != 0) {
            resetAccumulation();
        }
        m_lastGeomHash = 0;
        m_lastStructureHash = 0;
        m_lastTriCount = 0;
        buildAndUpload();
        return;
    }

    const uint64_t accelStructureHash = structureHash();
    const uint64_t geomHash = geometryHash();
    if (accelStructureHash == m_lastStructureHash && geomHash == m_lastGeomHash && triCount == m_lastTriCount) {
        return;
    }

    const bool needsRebuild =
        accelStructureHash != m_lastStructureHash || triCount != m_lastTriCount || !m_bvh.canRefit(triCount);
    m_lastStructureHash = accelStructureHash;
    m_lastGeomHash = geomHash;
    m_lastTriCount = triCount;
    if (needsRebuild) {
        buildAndUpload();
    } else {
        refitAndUpload();
    }
    resetAccumulation();
}

void SceneRayTracer::buildAndUpload() {
    std::vector<Raytrace::WorldTriangle> triangles;
    gather(triangles);
    uploadLights();
    m_bvh.build(triangles);
    if (m_bvh.isEmpty()) {
        return;
    }

    const auto& gpuTriangles = m_bvh.getGpuTris();
    const auto& gpuNodes = m_bvh.getNodes();
    ensureSSBOs(gpuTriangles.size(), gpuNodes.size());

    m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_triSsb);
    m_g->glBufferSubData(
        GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(gpuTriangles.size() * sizeof(GpuTri)), gpuTriangles.data());

    m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_nodeSsb);
    m_g->glBufferSubData(
        GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(gpuNodes.size() * sizeof(GpuBvhNode)), gpuNodes.data());

    m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SceneRayTracer::refitAndUpload() {
    std::vector<Raytrace::WorldTriangle> triangles;
    gather(triangles);
    uploadLights();
    m_bvh.refit(triangles);
    if (m_bvh.isEmpty()) {
        return;
    }

    const auto& gpuTriangles = m_bvh.getGpuTris();
    const auto& gpuNodes = m_bvh.getNodes();
    ensureSSBOs(gpuTriangles.size(), gpuNodes.size());

    m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_triSsb);
    m_g->glBufferSubData(
        GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(gpuTriangles.size() * sizeof(GpuTri)), gpuTriangles.data());

    m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_nodeSsb);
    m_g->glBufferSubData(
        GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(gpuNodes.size() * sizeof(GpuBvhNode)), gpuNodes.data());

    m_g->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SceneRayTracer::renderGpu(int w, int h, const Camera* camera) {
    const float aspect      = static_cast<float>(w) / static_cast<float>(h);
    const glm::mat4 proj    = glm::perspective(glm::radians(camera->fov), aspect, 0.1f, kFarClip);
    const glm::mat4 invView = glm::inverse(camera->getViewMatrix());
    const glm::mat4 invProj = glm::inverse(proj);

    m_shader->use();

    m_g->glBindImageTexture(
        0,           // image unit
        m_outTex,    // texture object
        0,           // mip level
        GL_FALSE,    // layered
        0,           // layer
        GL_READ_WRITE,
        GL_RGBA16F
    );

    m_g->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_triSsb);
    m_g->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_nodeSsb);
    m_g->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_lightSsb);

    m_shader->setVec3("uCamPos", camera->position);
    m_shader->setMat4("uInvView", invView);
    m_shader->setMat4("uInvProj", invProj);

    m_shader->setUVec2("uSize",
        static_cast<unsigned int>(w),
        static_cast<unsigned int>(h));

    m_shader->setUInt("uRoot",
        m_bvh.getRoot());

    m_shader->setUInt("uNumTris",
        static_cast<unsigned int>(m_bvh.getGpuTris().size()));

    m_shader->setUInt("uNumBvh",
        static_cast<unsigned int>(m_bvh.getNodes().size()));

    m_shader->setUInt("uNumLights",
        static_cast<unsigned int>(m_lightCount));

    m_shader->setUInt("uAccumFrames",
        static_cast<unsigned int>(m_accumulatedFrames));

    const bool globalLight =
        AppSettings::getInstance().getGroup<GraphicsSettings>().enableGlobalLight;
    m_shader->setInt("uEnableGlobalLight", globalLight ? 1 : 0);

    const unsigned int groupsX = (static_cast<unsigned int>(w) + 7u) / 8u;
    const unsigned int groupsY = (static_cast<unsigned int>(h) + 7u) / 8u;

    m_shader->dispatch(
        groupsX, groupsY, 1,
        GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT
    );
}

void SceneRayTracer::presentOutput(int fbWidth, int fbHeight) {
    m_g->glDisable(GL_DEPTH_TEST);
    m_g->glViewport(0, 0, fbWidth, fbHeight);
    m_g->glUseProgram(m_present);
    m_g->glActiveTexture(GL_TEXTURE0);
    m_g->glBindTexture(GL_TEXTURE_2D, m_outTex);
    m_g->glBindVertexArray(m_fsqVao);
    m_g->glDrawArrays(GL_TRIANGLES, 0, 3);
    m_g->glBindVertexArray(0);
    m_g->glBindTexture(GL_TEXTURE_2D, 0);
    m_g->glUseProgram(0);
    m_g->glEnable(GL_DEPTH_TEST);
}

SceneRayTracer::SceneRayTracer(SceneManager* sm, QOpenGLFunctions_4_5_Core* gl) : m_sm(sm), m_g(gl) {
    ensurePresentProgram();
    ensureComputeProgram();
    m_g->glGenVertexArrays(1, &m_fsqVao);
}

SceneRayTracer::~SceneRayTracer() {
    if (!m_g) {
        return;
    }
    if (m_present) {
        m_g->glDeleteProgram(m_present);
    }
    if (m_outTex) {
        m_g->glDeleteTextures(1, &m_outTex);
    }
    if (m_fsqVao) {
        m_g->glDeleteVertexArrays(1, &m_fsqVao);
    }
    if (m_triSsb) {
        m_g->glDeleteBuffers(1, &m_triSsb);
    }
    if (m_nodeSsb) {
        m_g->glDeleteBuffers(1, &m_nodeSsb);
    }
    if (m_lightSsb) {
        m_g->glDeleteBuffers(1, &m_lightSsb);
    }
}

void SceneRayTracer::render(
    int fbWidth, int fbHeight, Camera* camera, const std::optional<std::vector<ObjectSnapshot>>&, float internalScale) {
    if (!m_enabled || fbWidth < 1 || fbHeight < 1 || !m_g || !camera || !m_sm) {
        return;
    }
    if (!QOpenGLContext::currentContext()) {
        return;
    }

    ensurePresentProgram();
    ensureComputeProgram();
    if (!isUsable()) {
        return;
    }

    maybeRebuildAccel();
    if (m_bvh.isEmpty()) {
        m_g->glDisable(GL_DEPTH_TEST);
        m_g->glClearColor(kEmptySceneClearRed, kEmptySceneClearGreen, kEmptySceneClearBlue, 1.0f);
        m_g->glClear(GL_COLOR_BUFFER_BIT);
        m_g->glEnable(GL_DEPTH_TEST);
        return;
    }

    float scale = std::clamp(
        internalScale,
        GraphicsSettings::kMinRayTraceResolutionScale,
        GraphicsSettings::kMaxRayTraceResolutionScale);
    if (m_accumulatedFrames == 0u) {
        scale = std::min(scale, kGpuInteractiveMaxScale);
    }

    const int traceW = std::max(1, static_cast<int>(std::lround(static_cast<float>(fbWidth) * scale)));
    const int traceH = std::max(1, static_cast<int>(std::lround(static_cast<float>(fbHeight) * scale)));
    ensureOutputSize(traceW, traceH);

    const uint64_t currentViewHash = viewHash(traceW, traceH, scale, camera);
    const bool currentSunState = AppSettings::getInstance().getGroup<GraphicsSettings>().enableGlobalLight;

    if (currentViewHash != m_lastViewHash || currentSunState != m_lastEnableGlobalLight) {
        resetAccumulation();
        m_lastViewHash = currentViewHash;
        m_lastEnableGlobalLight = currentSunState;
    }

    if (m_accumulatedFrames >= kTemporalSampleCount) {
        presentOutput(fbWidth, fbHeight);
        return;
    }

    renderGpu(traceW, traceH, camera);

    ++m_accumulatedFrames;
    presentOutput(fbWidth, fbHeight);
}
