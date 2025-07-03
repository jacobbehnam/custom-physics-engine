#version 460 core
out vec4 FragColor;
layout (location = 2) in flat uint objectID;

uniform vec3 color;
layout(std140, binding = 1) uniform hoverData {
    int isHovered[1024];
};

void main()
{
    vec3 finalColor = color;
    if (isHovered[objectID] != 0)
        finalColor = mix(color, vec3(1.0, 1.0, 1.0), 0.5); // lighten when hovered

    FragColor = vec4(finalColor, 1.0);
}