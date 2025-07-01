#include "ResourceManager.h"
#include <graphics/utils/MathUtils.h>
#include <fstream>
#include <sstream>
#include <unordered_map>

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
    return &shaders.find(name)->second;
}

Mesh* ResourceManager::getMesh(const std::string &name) {
    return &meshes.find(name)->second;
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


std::unordered_map<std::string, Mesh> ResourceManager::meshes;
std::unordered_map<std::string, Shader> ResourceManager::shaders;