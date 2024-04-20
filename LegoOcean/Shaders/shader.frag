#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 worldPos;

layout(location = 0) out vec4 outColor;

void main() {
    if (worldPos.y > 10.0f) {
        outColor = vec4(fragColor, 1.0);
    }
    else {
        outColor = vec4(1.0);
    }
}