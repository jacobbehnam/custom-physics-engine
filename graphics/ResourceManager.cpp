#include "ResourceManager.h"

Shader* ResourceManager::LoadShader(const std::string &vShaderPath, const std::string &fShaderPath, const std::string &name) {
    return &shaders.try_emplace(name, vShaderPath, fShaderPath).first->second;
}

Mesh* ResourceManager::LoadMesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& idx, const std::string &name) {
    return &meshes.try_emplace(name, verts, idx).first->second;
}

Shader& ResourceManager::GetShader(const std::string &name) {
    return shaders.find(name)->second;
}

Mesh& ResourceManager::GetMesh(const std::string &name) {
    return meshes.find(name)->second;
}

std::unordered_map<std::string, Mesh> ResourceManager::meshes;
std::unordered_map<std::string, Shader> ResourceManager::shaders;