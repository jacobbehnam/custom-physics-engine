#include "UniformBuffer.h"
#include <glad/glad.h>

UniformBuffer::UniformBuffer(unsigned int size, unsigned int bindingPoint)
    : binding(bindingPoint), bufferSize(size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, id);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

UniformBuffer::~UniformBuffer() {
    glDeleteBuffers(1, &id);
}

void UniformBuffer::bind() const {
    glBindBuffer(GL_UNIFORM_BUFFER, id);
}

void UniformBuffer::updateData(const void* data, unsigned int size, unsigned int offset) {
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}