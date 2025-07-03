#version 460 core
layout (location = 0) in vec3 aPos;
layout(location = 2) in uint aObjectID;
flat out uint objectID;

layout(std140, binding=0) uniform cameraMatrices {
    mat4 projection;
    mat4 view;
};

uniform mat4 model;

void main()
{
    objectID = aObjectID;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}