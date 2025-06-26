#pragma once
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>

class Shader {
public:
    Shader(const std::string &vertexPath, const std::string &fragmentPath);

    void use() const;

    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    void setVec3(const std::string &name, const glm::vec3 &vec) const;
private:
    unsigned int ID;
    std::string loadFile(const std::string &path) const;
    unsigned int compileShader(GLenum type, const std::string &source) const;
};
