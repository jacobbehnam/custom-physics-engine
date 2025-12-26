#version 450 core
out vec4 FragColor;
flat in uint fragObjectID;
flat in vec3 baseColor;

layout(std140, binding = 1) uniform hoverData {
    ivec4 isHovered[1024];
};

void main()
{
    vec3 color = baseColor;
    if (isHovered[fragObjectID].x != 0)
        color = mix(vec3(1.0f), color, 0.2f); // lighten when hovered

    FragColor = vec4(color, 1.0f);
}