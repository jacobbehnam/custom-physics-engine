#version 450 core
out vec4 FragColor;

in vec3 FragPos;
flat in uint fragObjectID;

layout(std140, binding = 1) uniform hoverData {
    ivec4 isHovered[1024];
};

layout(std140, binding = 2) uniform selectData {
    ivec4 isSelected[1024];
};

void main()
{
    float tileSize = 100.0;
    float fX = floor(FragPos.x / tileSize);
    float fZ = floor(FragPos.z / tileSize);

    vec3 baseColor;
    if (mod(fX + fZ, 2.0) == 0.0) {
        baseColor = vec3(0.15, 0.15, 0.15);
    } else {
        baseColor = vec3(0.25, 0.25, 0.25);
    }

    float dist = length(FragPos.xz);
    float fogStart = 100000.0;
    float fogEnd = 290000.0;
    float fogFactor = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    vec3 skyColor = vec3(0.2, 0.3, 0.3);

    vec3 finalColor = mix(baseColor, skyColor, fogFactor);

    if (isSelected[fragObjectID].x != 0) {
        finalColor = mix(vec3(1.0, 0.0, 0.0), finalColor, 0.2);
    }
    else if (isHovered[fragObjectID].x != 0) {
        finalColor = mix(vec3(1.0), finalColor, 0.2);
    }

    FragColor = vec4(finalColor, 1.0);
}