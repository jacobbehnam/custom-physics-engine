#pragma once
#include <vector>
#include <glm/vec3.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();
    void draw() const;
private:
    unsigned int VAO, VBO, EBO, indexCount;
};

