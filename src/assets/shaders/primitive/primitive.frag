#version 450 core
out vec4 FragColor;
flat in uint fragObjectID;

uniform vec3 color;
layout(std140, binding = 1) uniform hoverData {
    ivec4 isHovered[1024];
};

layout(std140, binding = 2) uniform selectData {
    ivec4 isSelected[1024];
};

void main()
{
    vec3 finalColor = color;

    if (isSelected[fragObjectID].x != 0)
        finalColor = mix(vec3(1.0f, 0.0f, 0.0f), color, 0.2f);
    else if (isHovered[fragObjectID].x != 0)
        finalColor = mix(vec3(1.0f), color, 0.2f); // lighten when hovered

    FragColor = vec4(finalColor, 1.0f);
}