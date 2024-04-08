#version 450

layout(location = 0) in vec4 inOffset;

layout(location = 0) out vec3 fragColor;

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
    gl_Position = transform.P * transform.V * transform.M * vec4(positions[gl_VertexIndex] + ((gl_InstanceIndex % 10) - 5) * vec3(0.05, 0.0, 0.0) + ((gl_InstanceIndex / 10) - 5) * vec3(0.0, 0.0, 0.05) + inOffset.xyz, 1.0);
    fragColor = colors[2];
}