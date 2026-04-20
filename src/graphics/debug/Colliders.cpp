#include "Colliders.h"
#include "graphics/core/SceneManager.h"
#include "graphics/core/SceneObject.h"
#include "physics/PhysicsBody.h"
#include "physics/bounding/ICollider.h"
#include "graphics/core/ResourceManager.h"
#include <glm/gtc/matrix_transform.hpp>

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
    basicShader->use();
    basicShader->setVec3("color", glm::vec3(0.2f, 0.9f, 1.0f));

    std::vector<Rendering::InstanceData> instances;
    const auto& objects = sceneManager->getObjects();
    instances.reserve(objects.size());

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
        instances.emplace_back(model, obj->getObjectID(), glm::vec3(0.2f, 0.9f, 1.0f));
    }

    if (instances.empty()) return;

    GLint oldPolygonMode[2];
    GLfloat oldLineWidth = 1.0f;
    gl->glGetIntegerv(GL_POLYGON_MODE, oldPolygonMode);
    gl->glGetFloatv(GL_LINE_WIDTH, &oldLineWidth);

    gl->glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    gl->glLineWidth(1.0f);
    cubeMesh->drawInstanced(instances);
    gl->glPolygonMode(GL_FRONT, oldPolygonMode[0]);
    gl->glPolygonMode(GL_BACK, oldPolygonMode[1]);
    gl->glLineWidth(oldLineWidth);
}
