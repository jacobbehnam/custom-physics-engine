#include "UniformBuffer.h"
#include <iostream>
#include <glm/vec4.hpp>

UniformBuffer::UniformBuffer(unsigned int size, unsigned int bindingPoint)
    : binding(bindingPoint), bufferSize(size) {
    funcs = new QOpenGLFunctions_4_5_Core;
    funcs->initializeOpenGLFunctions();
    funcs->glGenBuffers(1, &id);
    funcs->glBindBuffer(GL_UNIFORM_BUFFER, id);
    funcs->glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    funcs->glBindBufferBase(GL_UNIFORM_BUFFER, binding, id);
    funcs->glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

UniformBuffer::~UniformBuffer() {
    funcs->glDeleteBuffers(1, &id);
}

void UniformBuffer::bind() const {
    funcs->glBindBuffer(GL_UNIFORM_BUFFER, id);
}

void UniformBuffer::updateData(const void* data, unsigned int size, unsigned int offset) {
    funcs->glBindBuffer(GL_UNIFORM_BUFFER, id);
    funcs->glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
    funcs->glBindBuffer(GL_UNIFORM_BUFFER, 0);
}