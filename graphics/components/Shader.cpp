#include "Shader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath) {
    funcs = new QOpenGLFunctions_4_5_Core;
    funcs->initializeOpenGLFunctions();

    std::string vertexCode = loadFile(vertexPath);
    std::string fragmentCode = loadFile(fragmentPath);

    unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexCode);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentCode);

    ID = funcs->glCreateProgram();
    funcs->glAttachShader(ID, vertex);
    funcs->glAttachShader(ID, fragment);
    funcs->glLinkProgram(ID);

    funcs->glDeleteShader(vertex);
    funcs->glDeleteShader(fragment);
}

std::string Shader::loadFile(const std::string &path) const {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: Could not open file " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int Shader::compileShader(GLenum type, const std::string &source) const {
    unsigned int shader = funcs->glCreateShader(type);
    const char* src = source.c_str();
    funcs->glShaderSource(shader, 1, &src, nullptr);
    funcs->glCompileShader(shader);

    int success;
    funcs->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        char infoLog[1024];
        funcs->glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
    }
    return shader;
}

void Shader::use() const {
    funcs->glUseProgram(ID);
}

void Shader::setBool(const std::string &name, bool value) const {
    funcs->glUniform1i(funcs->glGetUniformLocation(ID, name.c_str()), (int)value);
}
void Shader::setInt(const std::string &name, int value) const {
    funcs->glUniform1i(funcs->glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setFloat(const std::string &name, float value) const {
    funcs->glUniform1f(funcs->glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const {
    funcs->glUniformMatrix4fv(funcs->glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}
void Shader::setVec3(const std::string &name, const glm::vec3 &vec) const {
    funcs->glUniform3fv(funcs->glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}
