#version 450

layout(location = 0) in float inData;
//layout(location = 1) in vec3 inPosition;
//layout(location = 2) in vec3 inNorm;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 worldPos;
layout(location = 2) out vec3 normal;

vec3 positions[36] = vec3[](
    //Front
    vec3(-0.5, 0.5, -0.5),
    vec3(0.5, 0.5, -0.5),
    vec3(-0.5, -0.5,  -0.5),

    vec3(0.5, 0.5, -0.5),
    vec3(0.5, -0.5, -0.5),
    vec3(-0.5, -0.5, -0.5),

    //Back
    vec3(0.5, -0.5, 0.5),
    vec3(0.5, 0.5, 0.5),
    vec3(-0.5, 0.5,  0.5),

    vec3(-0.5, -0.5, 0.5),
    vec3(0.5, -0.5, 0.5),
    vec3(-0.5, 0.5, 0.5),

    //Top
    vec3(0.5, -0.5, 0.5),
    vec3(0.5, -0.5, -0.5),
    vec3(-0.5, -0.5, -0.5),

    vec3(0.5, -0.5, 0.5),
    vec3(-0.5, -0.5, -0.5),
    vec3(-0.5, -0.5, 0.5),

    //bottom
    vec3(0.5, 0.5, 0.5),
    vec3(0.5, 0.5, -0.5),
    vec3(-0.5, 0.5, -0.5),

    vec3(0.5, 0.5, 0.5),
    vec3(-0.5, 0.5, -0.5),
    vec3(-0.5, 0.5, 0.5),

    //Right
    vec3(0.5, -0.5, 0.5),
    vec3(0.5, 0.5, 0.5),
    vec3(0.5, 0.5, -0.5),

    vec3(0.5, -0.5, 0.5),
    vec3(0.5, 0.5, -0.5),
    vec3(0.5, -0.5, -0.5),

    //Left
    vec3(-0.5, -0.5, 0.5),
    vec3(-0.5, 0.5, 0.5),
    vec3(-0.5, 0.5, -0.5),

    vec3(-0.5, -0.5, 0.5),
    vec3(-0.5, 0.5, -0.5),
    vec3(-0.5, -0.5, -0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout(binding=0) uniform Transform {
    mat4 M;
    mat4 V;
    mat4 P;
} transform;

void main() {
    gl_PointSize = 10.0f;
    vec3 offset;
    offset.x = (int(inData) % 10) - 5;
    offset.y = (int(inData) / 100) - 5;
    offset.z = (int(inData) / 10) % 10 - 5;
    gl_Position = transform.P * transform.V * transform.M * vec4(positions[gl_VertexIndex] + offset, 1.0);
    //gl_Position = transform.P * transform.V * transform.M * vec4(inPosition, 1.0);
    //worldPos = (transform.M * vec4(inPosition, 1.0)).xyz;
    //normal = inNorm;
    fragColor = colors[2];
}