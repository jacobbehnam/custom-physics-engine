#include "Forces.h"
#include "graphics/core/SceneManager.h"
#include "physics/PhysicsBody.h"
#include "graphics/core/ResourceManager.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>

namespace {

constexpr float     kArrowStemMin          = 1.0f;
constexpr float     kArrowStemMax          = 3.5f;
constexpr float     kArrowRadiusMin        = 1.0f;
constexpr float     kArrowThicknessScale   = 0.08f;
constexpr float     kDefaultArrowStrengthT = 0.5f;
constexpr float     kForceEpsilon          = 1e-4f;
constexpr float     kMagSpanEpsilon        = 1e-6f;
constexpr float     kParallelAxisEpsilon   = 1e-3f;
constexpr glm::vec3 kArrowColor            = glm::vec3(1.0f, 1.0f, 0.0f);

glm::mat4 rotateFromYToDir(const glm::vec3& dir) {
    const glm::vec3 from(0.0f, 1.0f, 0.0f);
    const glm::vec3 to = glm::normalize(dir);
    const glm::vec3 crossA = glm::cross(from, to);
    const float cosA = glm::dot(from, to);

    if (glm::length(crossA) < kParallelAxisEpsilon) {
        if (cosA < 0.0f) {
            return glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        return glm::mat4(1.0f);
    }

    const float angle = std::acos(glm::clamp(cosA, -1.0f, 1.0f));
    return glm::rotate(glm::mat4(1.0f), angle, glm::normalize(crossA));
}

} // namespace

Forces::Forces(SceneManager* sceneManager) : sceneManager(sceneManager) {
    basicShader = ResourceManager::getShader("basic");
}

void Forces::draw(const std::optional<std::vector<ObjectSnapshot>>&) const {
    if (!enabled) return;

    auto* arrowMesh = ResourceManager::getMesh("gizmo_translate");
    if (!arrowMesh || !basicShader) return;

    const auto& objects = sceneManager->getObjects();

    m_arrowScratch.clear();
    if (m_arrowScratch.capacity() < objects.size()) {
        m_arrowScratch.reserve(objects.size());
    }

    float minMag = 0.0f;
    float maxMag = 0.0f;
    bool haveSpan = false;

    for (const auto& objPtr : objects) {
        SceneObject* obj = objPtr.get();
        auto* body = obj->getPhysicsBody();
        if (!body) continue;

        const glm::vec3 net = body->getNetForce(BodyLock::LOCK);
        const float netMag = glm::length(net);
        if (netMag < kForceEpsilon) continue;

        if (!haveSpan) {
            minMag = maxMag = netMag;
            haveSpan = true;
        } else {
            minMag = std::min(minMag, netMag);
            maxMag = std::max(maxMag, netMag);
        }

        const float radius = glm::compMax(glm::abs(obj->getScale())) * 0.5f;
        const glm::vec3 dir = glm::normalize(net);
        const glm::vec3 surfaceStart = body->getPosition(BodyLock::LOCK) + dir * radius;
        m_arrowScratch.push_back({obj->getObjectID(), net, netMag, surfaceStart, std::max(radius, kArrowRadiusMin)});
    }
    basicShader->use();

    const float magSpan = maxMag - minMag;
    const glm::vec3 renderOrigin = SceneObject::getRenderOrigin();

    m_instanceScratch.clear();
    if (m_instanceScratch.capacity() < m_arrowScratch.size()) {
        m_instanceScratch.reserve(m_arrowScratch.size());
    }

    for (const ArrowCpu& e : m_arrowScratch) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, e.startPos - renderOrigin);
        model = model * rotateFromYToDir(e.net);

        float t = kDefaultArrowStrengthT;
        if (magSpan > kMagSpanEpsilon) {
            t = (e.netMag - minMag) / magSpan;
        }
        const float lengthFactor = kArrowStemMin + t * (kArrowStemMax - kArrowStemMin);

        model = glm::scale(model, glm::vec3(e.radius * kArrowThicknessScale, e.radius * lengthFactor, e.radius * kArrowThicknessScale));
        m_instanceScratch.emplace_back(model, e.objectID, kArrowColor);
    }

    if (m_instanceScratch.empty()) return;
    arrowMesh->drawInstanced(m_instanceScratch);
}
