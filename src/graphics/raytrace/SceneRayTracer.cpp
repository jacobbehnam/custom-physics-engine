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
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <limits>
#include <span>
#include <sstream>
#include <thread>

namespace {

using Raytrace::GpuBvhNode;
using Raytrace::GpuTri;
using Raytrace::MaterialKind;

constexpr uint32_t kLeaf = 0x80000000u;
constexpr uint32_t kLeafStartMask = 0x0FFFFFFFu;
constexpr uint32_t kLeafCountShift = 28u;
constexpr uint32_t kLeafCountMask = 0x7u;
constexpr int kBvhStackSize = 64;
constexpr float kRayBias = 1.0e-3f;
constexpr float kFarClip = 300000.0f;
constexpr float kCpuFallbackMaxScale = 0.5f;
constexpr float kGpuInteractiveMaxScale = 0.6f;
const glm::vec3 kSunDir = glm::normalize(glm::vec3(0.35f, 0.9f, 0.22f));
const glm::vec3 kSunRadiance = glm::vec3(4.6f, 4.2f, 3.8f);

struct CpuHit {
    float t{std::numeric_limits<float>::max()};
    uint32_t triIndex{0};
    glm::vec3 bary{0.0f};
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec3 geomNormal{0.0f};
    glm::vec3 albedo{0.0f};
    bool hit{false};
};

static std::string readAll(const char* path) {
    std::ifstream file(path);
    if (!file) {
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

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

static std::array<float, 2> aabbInterval(
    const glm::vec3& origin, const glm::vec3& invDir, const glm::vec3& bmin, const glm::vec3& bmax) {
    const glm::vec3 t0 = (bmin - origin) * invDir;
    const glm::vec3 t1 = (bmax - origin) * invDir;
    const glm::vec3 tMin = glm::min(t0, t1);
    const glm::vec3 tMax = glm::max(t0, t1);
    return {
        std::max(tMin.x, std::max(tMin.y, tMin.z)),
        std::min(tMax.x, std::min(tMax.y, tMax.z)),
    };
}

static glm::vec3 geometricNormal(const GpuTri& tri) {
    return glm::normalize(glm::cross(glm::vec3(tri.v1) - glm::vec3(tri.v0), glm::vec3(tri.v2) - glm::vec3(tri.v0)));
}

static bool isFiniteVec3(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

static bool tryTriangle(
    const GpuTri& tri, const glm::vec3& origin, const glm::vec3& dir, float tMax, float& tHit, glm::vec3& bary) {
    const glm::vec3 v0(tri.v0);
    const glm::vec3 v1(tri.v1);
    const glm::vec3 v2(tri.v2);
    const glm::vec3 e1 = v1 - v0;
    const glm::vec3 e2 = v2 - v0;
    const glm::vec3 p = glm::cross(dir, e2);
    const float det = glm::dot(e1, p);
    if (std::abs(det) < 1.0e-6f) {
        return false;
    }

    const float invDet = 1.0f / det;
    const glm::vec3 t = origin - v0;
    const float u = glm::dot(t, p) * invDet;
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    const glm::vec3 q = glm::cross(t, e1);
    const float v = glm::dot(dir, q) * invDet;
    if (v < 0.0f || (u + v) > 1.0f) {
        return false;
    }

    const float hit = glm::dot(e2, q) * invDet;
    if (hit < 1.0e-4f || hit >= tMax) {
        return false;
    }

    tHit = hit;
    bary = glm::vec3(1.0f - u - v, u, v);
    return true;
}

static uint32_t decodeLeafCount(uint32_t packed) {
    return (packed >> kLeafCountShift) & kLeafCountMask;
}

static uint32_t decodeLeafStart(uint32_t packed) {
    return packed & kLeafStartMask;
}

static uint32_t triangleMaterialKind(const GpuTri& tri) {
    return static_cast<uint32_t>(tri.material.w + 0.5f);
}

static glm::vec3 evaluateMaterial(const GpuTri& tri, const glm::vec3& position) {
    const glm::vec3 base = glm::clamp(glm::vec3(tri.material), glm::vec3(0.02f), glm::vec3(0.98f));
    if (triangleMaterialKind(tri) != static_cast<uint32_t>(MaterialKind::Checkerboard)) {
        return base;
    }

    constexpr float tileSize = 100.0f;
    const float tx = std::floor(position.x / tileSize);
    const float tz = std::floor(position.z / tileSize);
    const bool dark = std::fmod(tx + tz, 2.0f) == 0.0f;
    const glm::vec3 a = glm::clamp(base * 0.7f, glm::vec3(0.02f), glm::vec3(0.95f));
    const glm::vec3 b = glm::clamp(base * 1.25f + glm::vec3(0.03f), glm::vec3(0.02f), glm::vec3(0.95f));
    return dark ? a : b;
}

static glm::vec3 skyRadiance(const glm::vec3& dir) {
    const float up = glm::clamp(dir.y * 0.5f + 0.5f, 0.0f, 1.0f);
    glm::vec3 sky = glm::mix(glm::vec3(0.80f, 0.84f, 0.92f), glm::vec3(0.20f, 0.38f, 0.78f), std::pow(up, 0.55f));
    if (dir.y < 0.0f) {
        sky = glm::mix(glm::vec3(0.12f, 0.13f, 0.15f), sky, glm::clamp(dir.y + 1.0f, 0.0f, 1.0f));
    }

    const float sun = std::max(glm::dot(dir, kSunDir), 0.0f);
    sky += glm::vec3(7.5f, 6.4f, 5.2f) * std::pow(sun, 1024.0f);
    sky += glm::vec3(1.8f, 1.3f, 0.9f) * std::pow(sun, 48.0f);
    return sky;
}

struct PcgRng {
    uint32_t state{0};

    explicit PcgRng(uint32_t seed) : state(seed ? seed : 1u) {}

    uint32_t nextU32() {
        state = state * 747796405u + 2891336453u;
        uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
        word = (word >> 22u) ^ word;
        return word;
    }

    float nextFloat() {
        return static_cast<float>(nextU32() & 0x00ffffffu) * (1.0f / 16777216.0f);
    }
};

static void buildBasis(const glm::vec3& n, glm::vec3& tangent, glm::vec3& bitangent) {
    if (std::abs(n.z) < 0.999f) {
        tangent = glm::normalize(glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), n));
    } else {
        tangent = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), n));
    }
    bitangent = glm::cross(n, tangent);
}

static glm::vec3 cosineHemisphere(const glm::vec3& n, PcgRng& rng) {
    const float u1 = rng.nextFloat();
    const float u2 = rng.nextFloat();
    const float r = std::sqrt(u1);
    const float phi = glm::two_pi<float>() * u2;
    const float x = r * std::cos(phi);
    const float y = r * std::sin(phi);
    const float z = std::sqrt(std::max(0.0f, 1.0f - u1));

    glm::vec3 tangent;
    glm::vec3 bitangent;
    buildBasis(n, tangent, bitangent);
    return glm::normalize(tangent * x + bitangent * y + n * z);
}

static bool traceClosest(
    const std::vector<GpuBvhNode>& nodes,
    const std::vector<GpuTri>& tris,
    uint32_t root,
    const glm::vec3& origin,
    const glm::vec3& dir,
    float tMax,
    CpuHit& outHit) {
    if (nodes.empty() || tris.empty()) {
        return false;
    }

    const glm::vec3 invDir = 1.0f / dir;
    std::array<uint32_t, kBvhStackSize> stack{};
    int sp = 0;

    const auto rootRange = aabbInterval(origin, invDir, nodes[root].bmin, nodes[root].bmax);
    if (rootRange[0] > rootRange[1] || rootRange[1] < 0.0f || rootRange[0] >= tMax) {
        return false;
    }
    stack[sp++] = root;

    CpuHit hit;
    hit.t = tMax;

    while (sp > 0) {
        const uint32_t nodeIndex = stack[--sp];
        const GpuBvhNode& node = nodes[nodeIndex];

        if ((node.c0 & kLeaf) != 0u) {
            const uint32_t count = decodeLeafCount(node.c0);
            const uint32_t start = decodeLeafStart(node.c0);
            for (uint32_t i = 0; i < count; ++i) {
                const uint32_t triIndex = start + i;
                float triT = hit.t;
                glm::vec3 bary(0.0f);
                if (!tryTriangle(tris[triIndex], origin, dir, hit.t, triT, bary)) {
                    continue;
                }

                hit.hit = true;
                hit.t = triT;
                hit.triIndex = triIndex;
                hit.bary = bary;
            }
            continue;
        }

        const uint32_t left = node.c0;
        const uint32_t right = node.c1;
        const auto leftRange = aabbInterval(origin, invDir, nodes[left].bmin, nodes[left].bmax);
        const auto rightRange = aabbInterval(origin, invDir, nodes[right].bmin, nodes[right].bmax);

        const bool hitLeft = leftRange[0] <= leftRange[1] && leftRange[1] >= 0.0f && leftRange[0] < hit.t;
        const bool hitRight = rightRange[0] <= rightRange[1] && rightRange[1] >= 0.0f && rightRange[0] < hit.t;

        if (hitLeft && hitRight) {
            const bool leftFirst = leftRange[0] <= rightRange[0];
            if (sp + 2 <= kBvhStackSize) {
                stack[sp++] = leftFirst ? right : left;
                stack[sp++] = leftFirst ? left : right;
            }
        } else if (hitLeft) {
            if (sp + 1 <= kBvhStackSize) {
                stack[sp++] = left;
            }
        } else if (hitRight) {
            if (sp + 1 <= kBvhStackSize) {
                stack[sp++] = right;
            }
        }
    }

    if (!hit.hit) {
        return false;
    }

    const GpuTri& tri = tris[hit.triIndex];
    hit.position = origin + dir * hit.t;
    glm::vec3 geomN = geometricNormal(tri);
    if (glm::dot(geomN, dir) > 0.0f) {
        geomN = -geomN;
    }
    glm::vec3 n = glm::normalize(
        hit.bary.x * glm::vec3(tri.n0) + hit.bary.y * glm::vec3(tri.n1) + hit.bary.z * glm::vec3(tri.n2));
    if (!isFiniteVec3(n) || glm::dot(n, n) < 1.0e-8f) {
        n = geomN;
    }
    if (glm::dot(n, dir) > 0.0f) {
        n = -n;
    }
    if (glm::dot(n, geomN) < 0.0f) {
        n = geomN;
    }
    hit.normal = n;
    hit.geomNormal = geomN;
    hit.albedo = evaluateMaterial(tri, hit.position);
    outHit = hit;
    return true;
}

static bool traceShadow(
    const std::vector<GpuBvhNode>& nodes,
    const std::vector<GpuTri>& tris,
    uint32_t root,
    const glm::vec3& origin,
    const glm::vec3& dir,
    float maxDistance) {
    CpuHit shadowHit;
    return traceClosest(nodes, tris, root, origin, dir, maxDistance, shadowHit);
}

static glm::vec3 tracePath(
    const std::vector<GpuBvhNode>& nodes,
    const std::vector<GpuTri>& tris,
    uint32_t root,
    const glm::vec3& origin,
    const glm::vec3& dir,
    PcgRng& rng);

static glm::vec3 shadePreview(
    const std::vector<GpuBvhNode>& nodes,
    const std::vector<GpuTri>& tris,
    uint32_t root,
    const glm::vec3& origin,
    const glm::vec3& dir,
    uint32_t seedBase) {
    glm::vec3 accum(0.0f);
    for (uint32_t i = 0; i < 2u; ++i) {
        PcgRng rng(seedBase + 0x9e3779b9u * (i + 1u));
        accum += tracePath(nodes, tris, root, origin, dir, rng);
    }
    return accum * 0.5f;
}

static glm::vec3 tracePath(
    const std::vector<GpuBvhNode>& nodes,
    const std::vector<GpuTri>& tris,
    uint32_t root,
    const glm::vec3& origin,
    const glm::vec3& dir,
    PcgRng& rng) {
    glm::vec3 throughput(1.0f);
    glm::vec3 radiance(0.0f);
    glm::vec3 rayOrigin = origin;
    glm::vec3 rayDir = dir;

    for (int bounce = 0; bounce < 2; ++bounce) {
        CpuHit hit;
        if (!traceClosest(nodes, tris, root, rayOrigin, rayDir, std::numeric_limits<float>::max(), hit)) {
            radiance += throughput * skyRadiance(rayDir);
            break;
        }

        const float nDotL = std::max(glm::dot(hit.normal, kSunDir), 0.0f);
        if (nDotL > 0.0f && !traceShadow(nodes, tris, root, hit.position + hit.geomNormal * kRayBias, kSunDir, kFarClip)) {
            radiance += throughput * hit.albedo * kSunRadiance * nDotL;
        }

        radiance += throughput * hit.albedo * skyRadiance(hit.normal) * 0.08f;
        throughput *= hit.albedo;

        const float rr = std::max(throughput.x, std::max(throughput.y, throughput.z));
        if (bounce > 0) {
            const float keep = std::clamp(rr, 0.1f, 0.95f);
            if (rng.nextFloat() > keep) {
                break;
            }
            throughput /= keep;
        }

        rayOrigin = hit.position + hit.geomNormal * kRayBias;
        rayDir = cosineHemisphere(hit.normal, rng);
    }

    return radiance;
}

static glm::vec3 primaryRayDirection(
    const glm::mat4& invView, const glm::mat4& invProj, int width, int height, float px, float py) {
    const float ndcX = (2.0f * px / static_cast<float>(width)) - 1.0f;
    const float ndcY = (2.0f * py / static_cast<float>(height)) - 1.0f;
    glm::vec4 clip(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 view = invProj * clip;
    view /= view.w;
    return glm::normalize(glm::vec3(invView * glm::vec4(glm::normalize(glm::vec3(view)), 0.0f)));
}

} // namespace

void SceneRayTracer::setRequireGpu(bool requireGpu) {
    if (m_requireGpu == requireGpu) {
        return;
    }
    m_requireGpu = requireGpu;
    resetAccumulation();
}

void SceneRayTracer::ensureComputeProgram() {
    if (m_computeAttempted || m_compute) {
        return;
    }
    m_computeAttempted = true;

    auto* ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        return;
    }
    const auto fmt = ctx->format();
    const bool supportsCompute =
        (fmt.majorVersion() > 4 || (fmt.majorVersion() == 4 && fmt.minorVersion() >= 3))
        || ctx->hasExtension(QByteArrayLiteral("GL_ARB_compute_shader"));
    if (!supportsCompute) {
        std::cerr << "[raytrace] compute shaders unavailable, CPU fallback will be used." << std::endl;
        return;
    }

    const std::string source = readAll("assets/shaders/raytrace/raytrace.comp");
    if (source.empty()) {
        std::cerr << "[raytrace] missing assets/shaders/raytrace/raytrace.comp" << std::endl;
        return;
    }

    const GLuint compute = compileShader(GL_COMPUTE_SHADER, source, "raytrace.comp", m_g);
    if (!compute) {
        return;
    }

    m_compute = m_g->glCreateProgram();
    m_g->glAttachShader(m_compute, compute);
    m_g->glLinkProgram(m_compute);
    m_g->glDeleteShader(compute);

    GLint linked = 0;
    m_g->glGetProgramiv(m_compute, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char log[2048];
        m_g->glGetProgramInfoLog(m_compute, sizeof(log) - 1, nullptr, log);
        std::cerr << "[raytrace] compute link failure\n" << log << std::endl;
        m_g->glDeleteProgram(m_compute);
        m_compute = 0;
        return;
    }

    m_lCam = m_g->glGetUniformLocation(m_compute, "uCamPos");
    m_lInvV = m_g->glGetUniformLocation(m_compute, "uInvView");
    m_lInvP = m_g->glGetUniformLocation(m_compute, "uInvProj");
    m_lSize = m_g->glGetUniformLocation(m_compute, "uSize");
    m_lRoot = m_g->glGetUniformLocation(m_compute, "uRoot");
    m_lNumT = m_g->glGetUniformLocation(m_compute, "uNumTris");
    m_lNumBvh = m_g->glGetUniformLocation(m_compute, "uNumBvh");
    m_lAccumFrames = m_g->glGetUniformLocation(m_compute, "uAccumFrames");
    m_lEnableSun = m_g->glGetUniformLocation(m_compute, "uEnableSun");
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

vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 col = texture(t, vTex).rgb;
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

    m_cpuPixels.assign(static_cast<size_t>(w) * static_cast<size_t>(h), glm::vec4(0.0f));
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

void SceneRayTracer::gather(std::vector<Raytrace::WorldTriangle>& out) {
    out.clear();
    const size_t triCount = quickTriCount();
    if (out.capacity() < triCount) {
        out.reserve(triCount);
    }

    for (const auto& owned : m_sm->getObjectStorage()) {
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

            out.push_back({
                w0,
                w1,
                w2,
                glm::normalize(n0),
                glm::normalize(n1),
                glm::normalize(n2),
                albedo,
                materialKind,
            });
        }
    }
}

size_t SceneRayTracer::quickTriCount() const {
    size_t total = 0;
    for (const auto& owned : m_sm->getObjectStorage()) {
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
    for (const auto& owned : m_sm->getObjectStorage()) {
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
    for (const auto& owned : m_sm->getObjectStorage()) {
        const SceneObject* object = owned.get();
        h = fnvAppend(h, object->getObjectID());

        const glm::mat4 model = object->getModelMatrix();
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                h = fnvAppendFloat(h, model[c][r]);
            }
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
    if (!m_cpuPixels.empty()) {
        std::fill(m_cpuPixels.begin(), m_cpuPixels.end(), glm::vec4(0.0f));
    }
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

SceneRayTracer::Backend SceneRayTracer::chooseBackend() const {
    return m_computeOk ? Backend::Gpu : Backend::Cpu;
}

void SceneRayTracer::renderGpu(int w, int h, const Camera* camera) {
    const glm::mat4 invView = glm::inverse(camera->getViewMatrix());
    const glm::mat4 invProj = glm::inverse(glm::perspective(glm::radians(camera->fov), static_cast<float>(w) / static_cast<float>(h), 0.1f, kFarClip));

    m_g->glUseProgram(m_compute);
    m_g->glBindImageTexture(0, m_outTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    m_g->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_triSsb);
    m_g->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_nodeSsb);

    m_g->glUniform3f(m_lCam, camera->position.x, camera->position.y, camera->position.z);
    m_g->glUniformMatrix4fv(m_lInvV, 1, GL_FALSE, glm::value_ptr(invView));
    m_g->glUniformMatrix4fv(m_lInvP, 1, GL_FALSE, glm::value_ptr(invProj));
    m_g->glUniform2ui(m_lSize, static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    m_g->glUniform1ui(m_lRoot, m_bvh.getRoot());
    m_g->glUniform1ui(m_lNumT, static_cast<unsigned int>(m_bvh.getGpuTris().size()));
    m_g->glUniform1ui(m_lNumBvh, static_cast<unsigned int>(m_bvh.getNodes().size()));
    m_g->glUniform1ui(m_lAccumFrames, m_accumulatedFrames);

    auto& vs = AppSettings::getInstance().getGroup<GraphicsSettings>();
    m_g->glUniform1i(m_lEnableSun, vs.enableSun ? 1 : 0);

    const unsigned int groupsX = (static_cast<unsigned int>(w) + 7u) / 8u;
    const unsigned int groupsY = (static_cast<unsigned int>(h) + 7u) / 8u;
    m_g->glDispatchCompute(groupsX, groupsY, 1u);
    m_g->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

void SceneRayTracer::uploadCpuTexture() {
    if (!m_outTex || m_cpuPixels.empty()) {
        return;
    }

    m_g->glBindTexture(GL_TEXTURE_2D, m_outTex);
    m_g->glTexSubImage2D(
        GL_TEXTURE_2D, 0, 0, 0, m_fbw, m_fbh, GL_RGBA, GL_FLOAT, m_cpuPixels.data());
    m_g->glBindTexture(GL_TEXTURE_2D, 0);
}

void SceneRayTracer::renderCpu(int w, int h, const Camera* camera) {
    const auto& nodes = m_bvh.getNodes();
    const auto& tris = m_bvh.getGpuTris();
    const glm::mat4 invView = glm::inverse(camera->getViewMatrix());
    const glm::mat4 invProj = glm::inverse(glm::perspective(glm::radians(camera->fov), static_cast<float>(w) / static_cast<float>(h), 0.1f, kFarClip));
    const uint32_t frameIndex = m_accumulatedFrames;

    const unsigned int hw = std::max(1u, std::thread::hardware_concurrency());
    const unsigned int workerCount = std::max(1u, std::min<unsigned int>(hw, static_cast<unsigned int>(h)));
    const int rowsPerWorker = std::max(1, (h + static_cast<int>(workerCount) - 1) / static_cast<int>(workerCount));

    auto renderRows = [&](int y0, int y1, uint32_t workerId) {
        for (int y = y0; y < y1; ++y) {
            for (int x = 0; x < w; ++x) {
                const uint32_t seed =
                    static_cast<uint32_t>(x * 1973 + y * 9277 + frameIndex * 26699u + workerId * 3181u + 0x68bc21ebu);
                PcgRng rng(seed);
                glm::vec3 sample(0.0f);
                if (frameIndex == 0u) {
                    const glm::vec3 rayDir = primaryRayDirection(
                        invView, invProj, w, h, static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
                    sample = shadePreview(nodes, tris, m_bvh.getRoot(), camera->position, rayDir, seed);
                } else {
                    const float jx = rng.nextFloat() - 0.5f;
                    const float jy = rng.nextFloat() - 0.5f;
                    const glm::vec3 rayDir = primaryRayDirection(
                        invView, invProj, w, h, static_cast<float>(x) + 0.5f + jx, static_cast<float>(y) + 0.5f + jy);
                    sample = tracePath(nodes, tris, m_bvh.getRoot(), camera->position, rayDir, rng);
                }

                const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x);
                glm::vec3 average = glm::vec3(m_cpuPixels[idx]);
                if (frameIndex == 0u) {
                    average = sample;
                } else {
                    average += (sample - average) * (1.0f / static_cast<float>(frameIndex + 1u));
                }
                m_cpuPixels[idx] = glm::vec4(average, 1.0f);
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(workerCount > 0 ? workerCount - 1 : 0);
    for (unsigned int worker = 1; worker < workerCount; ++worker) {
        const int y0 = static_cast<int>(worker) * rowsPerWorker;
        if (y0 >= h) {
            break;
        }
        const int y1 = std::min(h, y0 + rowsPerWorker);
        workers.emplace_back(renderRows, y0, y1, worker);
    }

    renderRows(0, std::min(h, rowsPerWorker), 0u);
    for (auto& worker : workers) {
        worker.join();
    }

    uploadCpuTexture();
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
    if (m_compute) {
        m_g->glDeleteProgram(m_compute);
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
}

void SceneRayTracer::render(
    int fbWidth, int fbHeight, Camera* camera, const std::optional<std::vector<ObjectSnapshot>>&, float internalScale) {
    if (fbWidth < 1 || fbHeight < 1 || !m_g || !camera || !m_sm) {
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
        m_g->glClearColor(0.40f, 0.52f, 0.66f, 1.0f);
        m_g->glClear(GL_COLOR_BUFFER_BIT);
        m_g->glEnable(GL_DEPTH_TEST);
        return;
    }

    const Backend backend = chooseBackend();
    float scale = std::clamp(internalScale, 0.25f, 1.0f);
    if (backend == Backend::Cpu) {
        scale = std::min(scale, kCpuFallbackMaxScale);
    } else if (m_accumulatedFrames == 0u) {
        scale = std::min(scale, kGpuInteractiveMaxScale);
    }

    const int traceW = std::max(1, static_cast<int>(std::lround(static_cast<float>(fbWidth) * scale)));
    const int traceH = std::max(1, static_cast<int>(std::lround(static_cast<float>(fbHeight) * scale)));
    ensureOutputSize(traceW, traceH);

    const uint64_t currentViewHash = viewHash(traceW, traceH, scale, camera);
    const bool currentSunState = AppSettings::getInstance().getGroup<GraphicsSettings>().enableSun;
    
    if (currentViewHash != m_lastViewHash || currentSunState != m_lastEnableSun) {
        resetAccumulation();
        m_lastViewHash = currentViewHash;
        m_lastEnableSun = currentSunState;
    }

    switch (backend) {
        case Backend::Gpu:
            renderGpu(traceW, traceH, camera);
            break;
        case Backend::Cpu:
            renderCpu(traceW, traceH, camera);
            break;
    }

    ++m_accumulatedFrames;
    presentOutput(fbWidth, fbHeight);
}
