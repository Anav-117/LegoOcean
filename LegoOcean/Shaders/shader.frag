#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 outColor;

vec3 lightPos = vec3(-10, 10, 40);

void main() {
    vec4 diffuse = max(0, dot(normalize(normal), normalize(lightPos - worldPos))) * vec4(fragColor, 1.0);
        
    vec4 ambient = 0.1*vec4(fragColor, 1.0);

    outColor = ambient+diffuse;
    
}