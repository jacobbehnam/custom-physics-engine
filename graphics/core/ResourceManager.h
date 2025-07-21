#pragma once
#include <unordered_map>
#include <string>
#include <graphics/components/Shader.h>
#include <graphics/components/Mesh.h>

class ResourceManager {
public:
    static void initialize(QOpenGLFunctions_4_5_Core* funcs);
    static void loadPrimitives();

    static Shader* loadShader(const std::string &vShaderPath, const std::string &fShaderPath, const std::string &name);
    static Mesh* loadMesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& idx, const std::string &name);
    static Mesh* loadMeshFromOBJ(const std::string& path, const std::string &name);

    static Shader* getShader(const std::string &name);
    static Mesh* getMesh(const std::string &name);

private:
    ResourceManager() = default;
    ~ResourceManager() = default;

    static bool loadOBJ(const std::string& path, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices);

    static void loadPrimCube();
    static void loadPrimSphere();

    inline static QOpenGLFunctions_4_5_Core* glFuncs = nullptr;
    inline static std::unordered_map<std::string, Shader> shaders;
    inline static std::unordered_map<std::string, Mesh> meshes;
};
