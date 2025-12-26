#version 450 core
layout (location = 0) in vec3 aPos;

// Instance attributes
layout (location = 2) in vec4 iModel0;
layout (location = 3) in vec4 iModel1;
layout (location = 4) in vec4 iModel2;
layout (location = 5) in vec4 iModel3;
layout (location = 6) in uint objectID;

flat out uint fragObjectID;

layout(std140, binding=0) uniform cameraMatrices {
    mat4 projection;
    mat4 view;
};

void main()
{
    mat4 model = mat4(iModel0, iModel1, iModel2, iModel3);
    fragObjectID = objectID;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}