#version 450

// Binding 0: Camera Data
layout(set = 0, binding = 0) uniform Camera {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Binding 1: Point Cloud Data (SSBO)
struct PositionVertex {
    vec3 position;
};

layout(std430, set = 0, binding = 1) readonly buffer PointBuffer {
    PositionVertex vertices[];
} pointData;

layout(location = 0) out vec3 fragColor;


const vec2 offsets[6] = vec2[](
vec2(-1.0, -1.0),
vec2( 1.0, -1.0),
vec2( 1.0,  1.0),
vec2(-1.0, -1.0),
vec2( 1.0,  1.0),
vec2(-1.0,  1.0)
);

void main() {
    vec3 centerPos = pointData.vertices[gl_InstanceIndex].position;

    float pointSize = 0.005; // Adjust this scale for larger/smaller points
    vec2 offset = offsets[gl_VertexIndex] * pointSize;

    vec4 viewPos = ubo.view * ubo.model * vec4(centerPos, 1.0);

    viewPos.xy += offset;

    gl_Position = ubo.proj * viewPos;

    fragColor = normalize(centerPos) * 0.5 + 0.5;
}