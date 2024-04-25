#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 outColor;

vec3 lightPos = vec3(10, 10, -10);

void main() {
    float diffuse = max(0, dot(normalize(normal), normalize(lightPos - worldPos)));
        
    outColor = vec4(diffuse*fragColor, 1.0);//vec4(1.0);
    
}