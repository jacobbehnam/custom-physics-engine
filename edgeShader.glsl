#version 460 core
layout (location = 0) in vec3 aPos;
out vec4 aCol;

uniform mat4 tmat;

void main() {
    gl_Position = tmat * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    aCol = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}
