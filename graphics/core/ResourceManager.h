#pragma once
#include <unordered_map>
#include <string>
#include <graphics/components/Shader.h>
#include <graphics/components/Mesh.h>

class ResourceManager {
public:
    static Shader* LoadShader(const std::string &vShaderPath, const std::string &fShaderPath, const std::string &name);
    static Mesh* LoadMesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& idx, const std::string &name);

    static Shader* GetShader(const std::string &name);
    static Mesh* GetMesh(const std::string &name);

private:
    ResourceManager() = default;
    ~ResourceManager() = default;

    static std::unordered_map<std::string, Shader> shaders;
    static std::unordered_map<std::string, Mesh> meshes;
};
