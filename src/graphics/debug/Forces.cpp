#include "Forces.h"
#include "graphics/core/SceneManager.h"
#include "physics/PhysicsBody.h"
#include "graphics/core/ResourceManager.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr float kArrowStemMin = 0.35f;
constexpr float kArrowStemMax = 2.4f;
constexpr float kForceEpsilon = 1e-4f;
constexpr float kMagSpanEpsilon = 1e-6f;
constexpr glm::vec3 kArrowColor(1.0f, 1.0f, 0.0f);

glm::mat4 rotateFromYToDir(const glm::vec3& dir) {
    const glm::vec3 from(0.0f, 1.0f, 0.0f);
    const glm::vec3 to = glm::normalize(dir);
    const glm::vec3 crossA = glm::cross(from, to);
    const float cosA = glm::dot(from, to);

    if (glm::length(crossA) < 1e-3f) {
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

void Forces::draw() const {
    if (!enabled) return;

    auto* arrowMesh = ResourceManager::getMesh("gizmo_translate");
    if (!arrowMesh || !basicShader) return;

    const auto& objects = sceneManager->getObjects();

    m_instanceScratch.clear();
    if (m_instanceScratch.capacity() < objects.size()) {
        m_instanceScratch.reserve(objects.size());
    }

    float minMag = 0.0f;
    float maxMag = 0.0f;
    bool haveSpan = false;

    for (const SceneObject* obj : objects) {
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
    }
    basicShader->use();

    const float magSpan = maxMag - minMag;
    for (SceneObject* obj : objects) {
        auto* body = obj->getPhysicsBody();
        if (!body) continue;
        const glm::vec3 net = body->getNetForce(BodyLock::LOCK);
        const float mag = glm::length(net);
        if (mag < kForceEpsilon) continue;

        float t = 0.5f;
        if (magSpan > kMagSpanEpsilon)
            t = (mag - minMag) / magSpan;
        const float lengthFactor = kArrowStemMin + t * (kArrowStemMax - kArrowStemMin);

        glm::mat4 model(1.0f);
        model = glm::translate(model, body->getPosition(BodyLock::LOCK));
        model = model * rotateFromYToDir(net);
        model = glm::scale(model, glm::vec3(1.0f, 1.5f * lengthFactor, 1.0f));

        m_instanceScratch.emplace_back(model, obj->getObjectID(), kArrowColor);
    }

    if (m_instanceScratch.empty()) return;
    arrowMesh->drawInstanced(m_instanceScratch);
}
