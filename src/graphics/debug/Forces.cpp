#include "Forces.h"
#include "graphics/core/SceneManager.h"
#include "physics/PhysicsBody.h"
#include "graphics/core/ResourceManager.h"
#include <glm/gtc/type_ptr.hpp>

static glm::mat4 rotateFromYToDir(const glm::vec3& dir) {
    glm::vec3 from = {0, 1, 0};
    glm::vec3 to = glm::normalize(dir);
    glm::vec3 crossA = glm::cross(from, to);
    float cosA = glm::dot(from, to);

    if (glm::length(crossA) < 1e-3f) {
        if (cosA < 0.0f) {
            return glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(1, 0, 0));
        }
        return glm::mat4(1.0f);
    }

    float angle = std::acos(glm::clamp(cosA, -1.0f, 1.0f));
    return glm::rotate(glm::mat4(1.0f), angle, glm::normalize(crossA));
}

Forces::Forces(SceneManager* sceneManager, QOpenGLFunctions_4_5_Core* glFuncs) 
    : sceneManager(sceneManager), gl(glFuncs) {
    basicShader = ResourceManager::getShader("basic");
    if (!basicShader) {
        basicShader = ResourceManager::loadShader("assets/shaders/primitive/primitive.vert", "assets/shaders/primitive/primitive.frag", "basic");
    }
}

void Forces::draw() const {
    if (!enabled) return;

    auto* arrowMesh = ResourceManager::getMesh("gizmo_translate");
    if (!arrowMesh || !basicShader) return;

    basicShader->use();
    basicShader->setVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));

    std::vector<Rendering::InstanceData> instances;
    const auto& objects = sceneManager->getObjects();

    for (SceneObject* obj : objects) {
        auto* body = obj->getPhysicsBody();
        if (!body) continue;

        auto forces = body->getAllForces(BodyLock::LOCK);
        glm::vec3 startPos = body->getPosition(BodyLock::LOCK);

        for (const auto& [name, force] : forces) {
            float mag = glm::length(force);
            if (mag < 0.01f) continue;

            glm::mat4 model(1.0f);
            model = glm::translate(model, startPos);
            model = model * rotateFromYToDir(force);
            
            // Gizmo-like scale, with slight length variation based on magnitude
            float lengthFactor = std::clamp(mag * 0.1f, 1.0f, 2.0f);
            // "almost same size of the gizmo move thing, like around 1/1.5 or something"
            model = glm::scale(model, glm::vec3(1.0f, 1.5f * lengthFactor, 1.0f));

            Rendering::InstanceData inst(model, obj->getObjectID(), glm::vec3(1.0f, 1.0f, 0.0f));
            instances.push_back(inst);
        }
    }

    if (!instances.empty()) {
        arrowMesh->drawInstanced(instances);
    }
}