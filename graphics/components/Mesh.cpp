#include "Mesh.h"
#include <vector>
#include <iostream>

Mesh::Mesh(const std::vector<Vertex> &verts, const std::vector<unsigned int> &idx, QOpenGLFunctions_4_5_Core* glFuncs)
    : indexCount(idx.size()), vertices(verts), indices(idx), funcs(glFuncs), localAABB(){
    assert(QOpenGLContext::currentContext() != nullptr && "GL context is NOT current during Mesh construction!");
    assert(funcs != nullptr);

    funcs->glGenVertexArrays(1, &VAO);
    assert(VAO != 0 && "VAO generation failed — is GL context current?");
    funcs->glGenBuffers(1, &VBO);
    funcs->glGenBuffers(1, &EBO);

    funcs->glBindVertexArray(VAO);
    funcs->glBindBuffer(GL_ARRAY_BUFFER, VBO);
    funcs->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    funcs->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    funcs->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    funcs->glEnableVertexAttribArray(0);
    funcs->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    funcs->glEnableVertexAttribArray(1);
    funcs->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    setupInstanceAttributes();
    createLocalAABB();
}

Mesh::~Mesh() {
    funcs->glDeleteVertexArrays(1, &VAO);
    funcs->glDeleteBuffers(1, &VBO);
    funcs->glDeleteBuffers(1, &EBO);
}

void Mesh::setupInstanceAttributes() {
    funcs->glGenBuffers(1, &instanceVBO);
    funcs->glBindVertexArray(VAO);
    funcs->glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    // Model matrix = 4 vec4s = locations 2, 3, 4, 5
    for (int i = 0; i < 4; ++i) {
        funcs->glEnableVertexAttribArray(2 + i);
        funcs->glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(sizeof(glm::vec4) * i));
        funcs->glVertexAttribDivisor(2 + i, 1);
    }

    // objectID = location 6
    funcs->glEnableVertexAttribArray(6);
    funcs->glVertexAttribIPointer(6, 1, GL_UNSIGNED_INT, sizeof(InstanceData), (void*)offsetof(InstanceData, objectID));
    funcs->glVertexAttribDivisor(6, 1);

    // color = location 7
    funcs->glEnableVertexAttribArray(7);
    funcs->glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, color));
    funcs->glVertexAttribDivisor(7, 1);

    funcs->glBindVertexArray(0);
}

void Mesh::createLocalAABB() {
    glm::vec3 minV(std::numeric_limits<float>::max()), maxV(-std::numeric_limits<float>::max());
    for (const Vertex& vertex : vertices) {
        minV = glm::min(minV, vertex.pos);
        maxV = glm::max(maxV, vertex.pos);
    }
    glm::vec3 center = (minV + maxV) * 0.5f;
    glm::vec3 halfExtents = (maxV - minV) * 0.5f;
    localAABB = Physics::Bounding::AABB(center, halfExtents);
}


void Mesh::draw() const {
    funcs->glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    funcs->glBindVertexArray(0);
}

void Mesh::drawInstanced(const std::vector<InstanceData>& instances) {
    assert(VAO != 0 && "asdVAO generation failed — is GL context current?");
    funcs->glBindVertexArray(VAO);

    funcs->glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    funcs->glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(InstanceData), instances.data(), GL_DYNAMIC_DRAW);

    funcs->glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr, instances.size());

    funcs->glBindVertexArray(0);
}

std::vector<Vertex> Mesh::getVertices() const {
    return vertices;
}

std::vector<unsigned int> Mesh::getIndices() const {
    return indices;
}
