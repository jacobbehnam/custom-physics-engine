#include "Colliders.h"
#include "graphics/core/SceneManager.h"
#include "graphics/core/SceneObject.h"
#include "physics/PhysicsBody.h"
#include "physics/bounding/ICollider.h"
#include "graphics/core/ResourceManager.h"
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr glm::vec3 kColliderColor(0.2f, 0.9f, 1.0f);
}

Colliders::Colliders(SceneManager* sceneManager, QOpenGLFunctions_4_5_Core* glFuncs) 
    : sceneManager(sceneManager), gl(glFuncs) {
    basicShader = ResourceManager::getShader("basic");
    if (!basicShader) {
        basicShader = ResourceManager::loadShader("assets/shaders/primitive/primitive.vert", "assets/shaders/primitive/primitive.frag", "basic");
    }
    cubeMesh = ResourceManager::getMesh("prim_cube");
}

void Colliders::draw() const {
    if (!enabled) return;

    if (!basicShader || !cubeMesh) return;

    const auto& objects = sceneManager->getObjects();
    m_instanceScratch.clear();
    if (m_instanceScratch.capacity() < objects.size()) {
        m_instanceScratch.reserve(objects.size());
    }

    for (SceneObject* obj : objects) {
        auto* body = obj->getPhysicsBody();
        if (!body) continue;
        auto* col = body->getCollider();
        if (!col) continue;

        auto worldMatrix = body->getWorldTransform(BodyLock::LOCK);
        auto worldCol = col->getTransformed(worldMatrix);
        if (!worldCol) continue;

        glm::vec3 minBound = worldCol->getAABBMin();
        glm::vec3 maxBound = worldCol->getAABBMax();
        const glm::vec3 center = (minBound + maxBound) * 0.5f;
        const glm::vec3 extents = maxBound - minBound;

        glm::mat4 model(1.0f);
        model = glm::translate(model, center);
        model = glm::scale(model, extents);
        m_instanceScratch.emplace_back(model, obj->getObjectID(), kColliderColor);
    }

    if (m_instanceScratch.empty()) return;

    basicShader->use();

    GLint oldPolygonMode[2];
    GLfloat oldLineWidth = 1.0f;
    gl->glGetIntegerv(GL_POLYGON_MODE, oldPolygonMode);
    gl->glGetFloatv(GL_LINE_WIDTH, &oldLineWidth);

    gl->glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    gl->glLineWidth(1.0f);
    cubeMesh->drawInstanced(m_instanceScratch);
    gl->glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(oldPolygonMode[0]));
    gl->glLineWidth(oldLineWidth);
}
