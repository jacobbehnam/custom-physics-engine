#include "PathTraces.h"
#include "physics/PhysicsBody.h"
#include "graphics/core/ResourceManager.h"
#include "graphics/core/SceneManager.h"

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

        const std::vector<ObjectSnapshot> snapshots = body->getAllFrames(BodyLock::LOCK);
        if (snapshots.size() < 2) continue;

        float latestTime = snapshots.back().time;
        float startTime = latestTime - timeWindow;

        points.clear();
        
        int startIdx = (int)snapshots.size() - 1;
        while (startIdx > 0 && snapshots[startIdx].time >= startTime) {
            startIdx--;
        }

        int count = (int)snapshots.size() - startIdx;
        points.reserve(count);
        
        for (int i = startIdx; i < snapshots.size(); ++i) {
            points.push_back(snapshots[i].position);
        }

        if (points.size() < 2) continue;

        gl->glBindBuffer(GL_ARRAY_BUFFER, vbo);
        gl->glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_DYNAMIC_DRAW);

        gl->glDrawArrays(GL_LINE_STRIP, 0, points.size());
    }

    gl->glBindVertexArray(0);
    gl->glLineWidth(oldLineWidth);
}
