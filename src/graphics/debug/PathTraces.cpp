#include "PathTraces.h"
#include "physics/PhysicsBody.h"
#include "graphics/core/ResourceManager.h"
#include "graphics/core/SceneManager.h"
#include "graphics/core/SceneObject.h"

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
    
    float oldLineWidth;
    gl->glGetFloatv(GL_LINE_WIDTH, &oldLineWidth);
    
    gl->glLineWidth(1.0f);

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

        int startIdx = static_cast<int>(snapshots.size() - 1);
        while (startIdx > 0 && snapshots[static_cast<size_t>(startIdx)].time >= startTime) {
            startIdx--;
        }

        const int count = static_cast<int>(snapshots.size()) - startIdx;
        points.reserve(static_cast<size_t>(count));

        for (int i = startIdx; i < static_cast<int>(snapshots.size()); ++i) {
            points.push_back(snapshots[static_cast<size_t>(i)].position - renderOrigin);
        }

        if (points.size() < 2) return;

        this->gl->glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
        this->gl->glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_DYNAMIC_DRAW);

        this->gl->glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(points.size()));
        });
    }

    gl->glBindVertexArray(0);
    gl->glLineWidth(oldLineWidth);
}
