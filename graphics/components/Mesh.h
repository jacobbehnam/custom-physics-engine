#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <physics/bounding/AABB.h>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;

    bool operator==(const Vertex& other) const {
        return pos == other.pos &&
               normal == other.normal;
    }
};

struct InstanceData {
    glm::mat4 model;
    uint32_t objectID;
    glm::vec3 color;
};

namespace std {
    template <>
    struct hash<Vertex> {
        size_t operator()(const Vertex& v) const {
            size_t h1 = hash<float>()(v.pos.x) ^ (hash<float>()(v.pos.y) << 1) ^ (hash<float>()(v.pos.z) << 2);
            size_t h2 = hash<float>()(v.normal.x)   ^ (hash<float>()(v.normal.y)   << 1) ^ (hash<float>()(v.normal.z) << 2);
            return h1 ^ (h2 << 1);
        }
    };
}

class Mesh {
public:
    Mesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& idx);
    ~Mesh();
    void draw() const;
    void drawInstanced(const std::vector<InstanceData>& instances) const;

    std::vector<Vertex> getVertices() const;
    std::vector<unsigned int> getIndices() const;
    const Physics::Bounding::AABB& getLocalAABB() const { return localAABB; }
private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Physics::Bounding::AABB localAABB;
    unsigned int VAO, VBO, instanceVBO, EBO, indexCount;

    void setupInstanceAttributes();
    void createLocalAABB();
};

