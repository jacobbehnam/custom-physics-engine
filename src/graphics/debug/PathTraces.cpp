#include <algorithm>
#include <iterator>

#include "PathTraces.h"
#include "physics/PhysicsBody.h"
#include "graphics/core/ResourceManager.h"
#include "graphics/core/SceneManager.h"
#include "graphics/core/SceneObject.h"

namespace {
constexpr int kMaxTrailPointsPerObject = 4096;
}

PathTraces::PathTraces(SceneManager* sceneManager, QOpenGLFunctions_4_5_Core* glFuncs) : sceneManager(sceneManager), gl(glFuncs) {
    gl->glGenVertexArrays(1, &vao);
    gl->glGenBuffers(1, &vbo);

    gl->glBindVertexArray(vao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    gl->glEnableVertexAttribArray(0);
    gl->glBindVertexArray(0);

    traceShader = ResourceManager::getShader("pathtrace");
    if (!traceShader) {
        traceShader = ResourceManager::loadShader("assets/shaders/debug/pathtrace.vert", "assets/shaders/debug/pathtrace.frag", "pathtrace");
    }
}

PathTraces::~PathTraces() {
    if (vao) gl->glDeleteVertexArrays(1, &vao);
    if (vbo) gl->glDeleteBuffers(1, &vbo);
}

void PathTraces::draw() const {
    if (!traceShader) return;
    if (!enabled) return;

    const auto& objects = sceneManager->getObjects();

    traceShader->use();

    gl->glBindVertexArray(vao);
    
    const GLboolean depthWasEnabled = gl->glIsEnabled(GL_DEPTH_TEST);
    if (depthWasEnabled) {
        gl->glDisable(GL_DEPTH_TEST);
    }

    float oldLineWidth;
    gl->glGetFloatv(GL_LINE_WIDTH, &oldLineWidth);
    
    gl->glLineWidth(2.0f);

    std::vector<glm::vec3> points;
    for (SceneObject* obj : objects) {
        Physics::PhysicsBody* body = obj->getPhysicsBody();
        if (!body) continue;

        const glm::vec3 renderOrigin = SceneObject::getRenderOrigin();
        body->withFrames(BodyLock::LOCK, [this, &points, renderOrigin](const std::vector<ObjectSnapshot>& snapshots) {
        if (snapshots.size() < 2) return;

        const float latestTime = snapshots.back().time;
        const float startTime = latestTime - this->timeWindow;

        points.clear();

        const auto startIt = std::lower_bound(snapshots.begin(), snapshots.end(), startTime,
            [](const ObjectSnapshot& snapshot, float time) {
                return snapshot.time < time;
            });
        int startIdx = static_cast<int>(std::distance(snapshots.begin(), startIt));
        int totalCount = static_cast<int>(snapshots.size()) - startIdx;
        if (totalCount < 2) {
            startIdx = std::max(0, static_cast<int>(snapshots.size()) - 2);
            totalCount = static_cast<int>(snapshots.size()) - startIdx;
        }
        const int stride = std::max(1, totalCount / kMaxTrailPointsPerObject);
        points.reserve(static_cast<size_t>(std::min(totalCount, kMaxTrailPointsPerObject + 1)));

        int lastDrawnIndex = -1;
        for (int i = startIdx; i < static_cast<int>(snapshots.size()); i += stride) {
            points.push_back(snapshots[static_cast<size_t>(i)].position - renderOrigin);
            lastDrawnIndex = i;
        }
        if (lastDrawnIndex != static_cast<int>(snapshots.size()) - 1) {
            points.push_back(snapshots.back().position - renderOrigin);
        }

        if (points.size() < 2) return;

        this->gl->glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
        this->gl->glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_DYNAMIC_DRAW);

        this->gl->glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(points.size()));
        });
    }

    gl->glBindVertexArray(0);
    gl->glLineWidth(oldLineWidth);
    if (depthWasEnabled) {
        gl->glEnable(GL_DEPTH_TEST);
    }
}
