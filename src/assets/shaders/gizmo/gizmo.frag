#version 450 core
out vec4 FragColor;
flat in vec3 baseColor;

void main() {
    FragColor = vec4(baseColor, 1.0);
}