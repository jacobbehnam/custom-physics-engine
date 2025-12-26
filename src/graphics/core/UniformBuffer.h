#pragma once
#include <QOpenGLFunctions_4_5_Core>

class UniformBuffer {
public:
    UniformBuffer(unsigned int size, unsigned int bindingPoint, QOpenGLFunctions_4_5_Core* glFuncs);
    ~UniformBuffer();

    void bind() const;
    void updateData(const void* data, unsigned int size, unsigned int offset = 0);

private:
    QOpenGLFunctions_4_5_Core* funcs;
    unsigned int id;
    unsigned int binding;
    unsigned int bufferSize;
};
