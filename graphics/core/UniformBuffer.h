#pragma once
class UniformBuffer {
public:
    UniformBuffer(unsigned int size, unsigned int bindingPoint);
    ~UniformBuffer();

    void bind() const;
    void updateData(const void* data, unsigned int size, unsigned int offset = 0);

private:
    unsigned int id;
    unsigned int binding;
    unsigned int bufferSize;
};