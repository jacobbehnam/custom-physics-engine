#version 450 core
layout (location = 0) in vec3 aPos;

layout(std140, binding=0) uniform cameraMatrices {
    mat4 projection;
    mat4 view;
};

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
}
