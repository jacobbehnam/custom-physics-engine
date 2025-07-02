#version 460 core
out vec4 FragColor;

uniform vec3 color;
uniform bool isHovered;

void main()
{
    vec3 finalColor = color;
    if (isHovered)
        finalColor = mix(color, vec3(1.0, 1.0, 1.0), 0.5); // lighten when hovered

    FragColor = vec4(finalColor, 1.0);
}