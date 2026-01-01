#version 450

layout(set = 0, binding = 0) uniform Camera {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 fragColor;

// Procedural Cube Data
const vec3 positions[8] = vec3[8](
    vec3(0,0,0), vec3(1,0,0), vec3(0,1,0), vec3(1,1,0),
    vec3(0,0,1), vec3(1,0,1), vec3(0,1,1), vec3(1,1,1)
);

// Standard triangle indices for a cube
const int indices[36] = int[36](
    0,2,1, 1,2,3, // Bottom (z=0)
    4,5,6, 5,7,6, // Top (z=1)
    0,1,4, 1,5,4, // Front (y=0)
    2,6,3, 3,6,7, // Back (y=1)
    0,4,2, 2,4,6, // Left (x=0)
    1,3,5, 3,7,5  // Right (x=1)
);

void main() {
    // Retrieve position based on procedural index
    vec3 pos = positions[indices[gl_VertexIndex]];

    fragColor = pos;

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
}