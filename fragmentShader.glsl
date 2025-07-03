#version 460 core
out vec4 FragColor;
flat in uint fragObjectID;

uniform vec3 color;
layout(std140, binding = 1) uniform hoverData {
    ivec4 isHovered[1024];
};

void main()
{
    vec3 finalColor = color;

    if (isHovered[fragObjectID].x != 0)
        finalColor = vec3(1.0f,1.0f,1.0f); // lighten when hovered

    FragColor = vec4(finalColor, 1.0f);
}