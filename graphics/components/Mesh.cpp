#include "Mesh.h"
#include "glad/glad.h"
#include <vector>
#include <iostream>

Mesh::Mesh(const std::vector<Vertex> &verts, const std::vector<unsigned int> &idx)
    : indexCount(idx.size()), vertices(verts), indices(idx){

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    setupInstanceAttributes();
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Mesh::setupInstanceAttributes() {
    glGenBuffers(1, &instanceVBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    // Model matrix = 4 vec4s = locations 2, 3, 4, 5
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(2 + i);
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(2 + i, 1);
    }

    // objectID = location 6
    glEnableVertexAttribArray(6);
    glVertexAttribIPointer(6, 1, GL_UNSIGNED_INT, sizeof(InstanceData), (void*)offsetof(InstanceData, objectID));
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}


void Mesh::draw() const {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::drawInstanced(const std::vector<InstanceData>& instances) const {
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(InstanceData), instances.data(), GL_DYNAMIC_DRAW);

    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr, instances.size());

    glBindVertexArray(0);
}

std::vector<Vertex> Mesh::getVertices() const {
    return vertices;
}

std::vector<unsigned int> Mesh::getIndices() const {
    return indices;
}
