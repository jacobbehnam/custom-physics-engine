#include <algorithm>
#include <cmath>
#include <iterator>

#include "PathTraces.h"
#include "physics/PhysicsBody.h"
#include "graphics/core/ResourceManager.h"
#include "graphics/core/SceneManager.h"
#include "graphics/core/SceneObject.h"

namespace {
constexpr int   kMaxTrailPointsPerObject = 4096;
constexpr int   kMinTrailPointCount      = 2;
constexpr float kTraceLineWidth          = 2.0f;
constexpr float kFrameTimeEpsilon        = 1.0e-4f;

const ObjectSnapshot* findRenderedSnapshot(
    const std::optional<std::vector<ObjectSnapshot>>& renderSnapshots,
    const Physics::PhysicsBody* body) {
    if (!renderSnapshots) return nullptr;

    for (const ObjectSnapshot& snapshot : *renderSnapshots) {
        if (snapshot.body == body) {
            return &snapshot;
        }
    }
    return nullptr;
}
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
}

PathTraces::~PathTraces() {
    if (vao) gl->glDeleteVertexArrays(1, &vao);
    if (vbo) gl->glDeleteBuffers(1, &vbo);
}

void PathTraces::draw(const std::optional<std::vector<ObjectSnapshot>>& renderSnapshots) const {
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
    
    gl->glLineWidth(kTraceLineWidth);

    std::vector<glm::vec3> points;
    for (const auto& objPtr : objects) {
        SceneObject* obj = objPtr.get();
        Physics::PhysicsBody* body = obj->getPhysicsBody();
        if (!body) continue;

        const glm::vec3 renderOrigin = SceneObject::getRenderOrigin();
        const ObjectSnapshot* renderedSnapshot = findRenderedSnapshot(renderSnapshots, body);
        body->withFrames(BodyLock::LOCK, [this, &points, renderOrigin, renderedSnapshot](const std::vector<ObjectSnapshot>& snapshots) {
            if (snapshots.size() < kMinTrailPointCount) return;

            const float latestTime = renderedSnapshot ? renderedSnapshot->time : snapshots.back().time;
            const glm::vec3 latestPosition = renderedSnapshot ? renderedSnapshot->position : snapshots.back().position;
            const float startTime = latestTime - this->timeWindow;

            points.clear();

            const auto startIt = std::lower_bound(snapshots.begin(), snapshots.end(), startTime,
                [](const ObjectSnapshot& snapshot, float time) {
                    return snapshot.time < time;
                });
            const auto endIt = std::upper_bound(snapshots.begin(), snapshots.end(), latestTime,
                [](float time, const ObjectSnapshot& snapshot) {
                    return time < snapshot.time;
                });
            int startIdx = static_cast<int>(std::distance(snapshots.begin(), startIt));
            int endIdx = static_cast<int>(std::distance(snapshots.begin(), endIt));
            if (endIdx <= 0) return;

            int totalCount = endIdx - startIdx;
            if (totalCount < kMinTrailPointCount) {
                startIdx = std::max(0, endIdx - kMinTrailPointCount);
                totalCount = endIdx - startIdx;
            }
            const int stride = std::max(1, totalCount / kMaxTrailPointsPerObject);
            points.reserve(static_cast<size_t>(std::min(totalCount, kMaxTrailPointsPerObject + 1)));

            int lastDrawnIndex = -1;
            for (int i = startIdx; i < endIdx; i += stride) {
                points.push_back(snapshots[static_cast<size_t>(i)].position - renderOrigin);
                lastDrawnIndex = i;
            }
            if (lastDrawnIndex < 0
                || std::abs(snapshots[static_cast<size_t>(lastDrawnIndex)].time - latestTime) > kFrameTimeEpsilon) {
                points.push_back(latestPosition - renderOrigin);
            }

            if (points.size() < kMinTrailPointCount) return;

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
