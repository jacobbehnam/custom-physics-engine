#include "ComputeShader.h"
#include <fstream>
#include <sstream>

ComputeShader::ComputeShader(const std::string& computePath, QOpenGLFunctions_4_5_Core* glFuncs)
    : funcs(glFuncs)
{
    std::string source = loadFile(computePath);
    unsigned int comp = compileShader(GL_COMPUTE_SHADER, source);

    ID = funcs->glCreateProgram();
    funcs->glAttachShader(ID, comp);
    funcs->glLinkProgram(ID);

    funcs->glDeleteShader(comp);
}

ComputeShader::~ComputeShader() {
    funcs->glDeleteProgram(ID);
    for (auto ssbo : ssboIDs) {
        funcs->glDeleteBuffers(1, &ssbo);
    }
}

void ComputeShader::use() const {
    funcs->glUseProgram(ID);
}

unsigned int ComputeShader::compileShader(GLenum type, const std::string& source) const {
    unsigned int shader = funcs->glCreateShader(type);
    const char* src = source.c_str();
    funcs->glShaderSource(shader, 1, &src, nullptr);
    funcs->glCompileShader(shader);

    GLint success;
    funcs->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        funcs->glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::COMPUTE_SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

std::string ComputeShader::loadFile(const std::string &path) const {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: Could not open file " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int ComputeShader::createSSBO(const void *data, unsigned int size, unsigned int bindingPoint) {
    unsigned int ssbo;
    funcs->glGenBuffers(1, &ssbo);
    funcs->glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    funcs->glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY);
    funcs->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, ssbo);
    ssboIDs.push_back(ssbo);
    return ssbo;
}

void ComputeShader::dispatch(GLuint groupsX, GLuint groupsY, GLuint groupsZ) const {
    funcs->glDispatchCompute(groupsX, groupsY, groupsZ);
    funcs->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ComputeShader::setBool(const std::string& name, bool value) const {
    funcs->glUniform1i(funcs->glGetUniformLocation(ID, name.c_str()), (int)value);
}

void ComputeShader::setInt(const std::string& name, int value) const {
    funcs->glUniform1i(funcs->glGetUniformLocation(ID, name.c_str()), value);
}

void ComputeShader::setFloat(const std::string& name, float value) const {
    funcs->glUniform1f(funcs->glGetUniformLocation(ID, name.c_str()), value);
}

void ComputeShader::setMat4(const std::string& name, const glm::mat4& mat) const {
    funcs->glUniformMatrix4fv(funcs->glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void ComputeShader::setVec3(const std::string& name, const glm::vec3& vec) const {
    funcs->glUniform3fv(funcs->glGetUniformLocation(ID, name.c_str()), 1, &vec[0]);
}
