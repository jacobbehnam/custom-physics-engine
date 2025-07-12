#include "ResourceManager.h"
#include <graphics/utils/MathUtils.h>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/constants.hpp>

Shader* ResourceManager::loadShader(const std::string &vShaderPath, const std::string &fShaderPath, const std::string &name) {
    return &shaders.try_emplace(name, vShaderPath, fShaderPath).first->second;
}

Mesh* ResourceManager::loadMesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& idx, const std::string &name) {
    return &meshes.try_emplace(name, verts, idx).first->second;
}

Mesh *ResourceManager::loadMeshFromOBJ(const std::string &path, const std::string &name) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    if (loadOBJ(path, vertices, indices)) {
        return loadMesh(vertices, indices, name);
    }
    return nullptr;
}


Shader* ResourceManager::getShader(const std::string &name) {
    auto it = shaders.find(name);
    if (it != shaders.end()) {
        return &it->second;
    } else {
        return nullptr;
    }
}

Mesh* ResourceManager::getMesh(const std::string &name) {
    auto it = meshes.find(name);
    if (it != meshes.end()) {
        return &it->second;
    } else {
        return nullptr;
    }
}

// Expects triangulated obj files!
bool ResourceManager::loadOBJ(const std::string &path, std::vector<Vertex> &outVertices, std::vector<unsigned int> &outIndices) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;

    std::unordered_map<Vertex, unsigned int> vertexToIndex;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        } else if (type == "vt") {
            // glm::vec2 uv;
            // iss >> uv.x >> uv.y;
            // texCoords.push_back(uv);
        } else if (type == "vn") {
            glm::vec3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        } else if (type == "f") {
            std::string vertexStr;
            while (iss >> vertexStr) {
                std::istringstream vss(vertexStr);
                std::string idxStr;
                int posIdx = 0, uvIdx = 0, normIdx = 0;

                std::getline(vss, idxStr, '/');
                posIdx = std::stoi(idxStr);

                if (std::getline(vss, idxStr, '/') && !idxStr.empty())
                    uvIdx = std::stoi(idxStr);
                if (std::getline(vss, idxStr, '/'))
                    normIdx = std::stoi(idxStr);

                Vertex vertex{};
                vertex.pos = positions[posIdx - 1];
                //if (uvIdx) vertex.texCoord = texCoords[uvIdx - 1];
                if (normIdx) vertex.normal = normals[normIdx - 1];

                if (vertexToIndex.count(vertex) == 0) {
                    vertexToIndex[vertex] = static_cast<unsigned int>(outVertices.size());
                    outVertices.push_back(vertex);
                }

                outIndices.push_back(vertexToIndex[vertex]);
            }
        }
    }

    return true;
}

// Primitive loading
void ResourceManager::loadPrimitives() {
    loadPrimCube();
    loadPrimSphere();
    loadMeshFromOBJ("../Arrow.obj", "arrow");
    loadMeshFromOBJ("../Scale.obj", "scale");
    loadMeshFromOBJ("../Rotate.obj", "rotate");
}

void ResourceManager::loadPrimCube() {
    std::vector<Vertex> vertices = {
        { {-0.5f, -0.5f, -0.5f}, {0.0f,0.0f,0.0f}},
        { {0.5f, -0.5f, -0.5f}, {0.0f,0.0f,0.0f}},
        { {0.5f, 0.5f, -0.5f}, {0.0f,0.0f,0.0f}},
        { {-0.5f, 0.5f, -0.5f}, {0.0f,0.0f,0.0f}},
        { {-0.5f, -0.5f, 0.5f}, {0.0f,0.0f,0.0f}},
        { {0.5f, -0.5f, 0.5f}, {0.0f,0.0f,0.0f}},
        { {0.5f, 0.5f, 0.5f}, {0.0f,0.0f,0.0f}},
        { {-0.5f, 0.5f, 0.5f}, {0.0f,0.0f,0.0f}}
    };

    std::vector<unsigned int> indices {
        0, 1, 2,
        0, 3, 2,
        0, 1, 5,
        0, 4, 5,
        0, 4, 7,
        0, 3, 7,
        1, 2, 6,
        1, 5, 6,
        2, 3, 7,
        2, 6, 7,
        4, 5, 6,
        4, 7, 6
    };

    loadMesh(vertices, indices, "prim_cube");
}

void ResourceManager::loadPrimSphere() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    unsigned int latitudeSegments = 64;
    unsigned int longitudeSegments = 64;

    // 1) Generate vertices
    for (unsigned int y = 0; y <= latitudeSegments; ++y) {
        float v = float(y) / latitudeSegments;
        float theta = v * glm::pi<float>();

        for (unsigned int x = 0; x <= longitudeSegments; ++x) {
            float u = float(x) / longitudeSegments;
            float phi = u * glm::two_pi<float>();

            // spherical to Cartesian
            float sinT = std::sin(theta);
            glm::vec3 pos{
                sinT * std::cos(phi),
                std::cos(theta),
                sinT * std::sin(phi)
            };

            glm::vec3 normal = glm::normalize(pos);

            vertices.push_back({ pos, normal });
        }
    }

    // 2) Generate indices
    // we create quads between each (y, x) and (y+1, x+1) and split each quad into two tris
    for (unsigned int y = 0; y < latitudeSegments; ++y) {
        for (unsigned int x = 0; x < longitudeSegments; ++x) {
            // indices to the four corners of this quad
            unsigned int i0 = y * (longitudeSegments + 1) + x;
            unsigned int i1 = (y + 1) * (longitudeSegments + 1) + x;
            unsigned int i2 = (y + 1) * (longitudeSegments + 1) + (x + 1);
            unsigned int i3 = y * (longitudeSegments + 1) + (x + 1);

            // lower‐left triangle  (i0, i1, i2)
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            // upper‐right triangle (i0, i2, i3)
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
    loadMesh(vertices, indices, "prim_sphere");
}


std::unordered_map<std::string, Mesh> ResourceManager::meshes;
std::unordered_map<std::string, Shader> ResourceManager::shaders;