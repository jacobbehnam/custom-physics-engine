#pragma once
#include <vector>
#include <glm/vec3.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& idx);
    ~Mesh();
    void draw() const;

    std::vector<Vertex> getVertices() const;
    std::vector<unsigned int> getIndices() const;
private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO, indexCount;
};

