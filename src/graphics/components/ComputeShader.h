#pragma once
#include <string>
#include <glm/glm.hpp>
#include <QOpenGLFunctions_4_5_Core>
#include <iostream>

class ComputeShader {
public:
    ComputeShader(const std::string& computePath, QOpenGLFunctions_4_5_Core* glFuncs);
    ~ComputeShader();

    void use() const;

    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec3(const std::string& name, const glm::vec3& vec) const;

    unsigned int createSSBO(const void* data, unsigned int size, unsigned int bindingPoint);
    template<typename T>
    std::vector<T> readSSBO(unsigned int bufferID, size_t count) const {
        std::vector<T> result(count);
        funcs->glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);

        void* ptr = funcs->glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(T), GL_MAP_READ_BIT);
        if (ptr) {
            T* dataPtr = static_cast<T*>(ptr);
            std::copy(dataPtr, dataPtr + count, result.begin());
            funcs->glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        } else {
            std::cerr << "ComputeShader::readSSBO() — glMapBufferRange failed" << std::endl;
        }
        return result;
    }

    void dispatch(unsigned int groupsX, unsigned int groupsY = 1, unsigned int groupsZ = 1) const;

    unsigned int id() const { return ID; }

private:
    QOpenGLFunctions_4_5_Core* funcs;
    unsigned int ID;
    std::vector<unsigned int> ssboIDs;
    std::string loadFile(const std::string& path) const;
    unsigned int compileShader(GLenum type, const std::string& source) const;
};