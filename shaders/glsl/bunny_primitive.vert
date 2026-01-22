#version 450

// Binding 0: Camera/Scene Data
layout(set = 0, binding = 0) uniform SceneData {
    mat4 model;
    mat4 view;
    mat4 proj;
    float maxHeight;
    int colorMode;
} ubo;

// Binding 1: Point Cloud Data (SSBO)
// Matching C++ struct Point with alignas(16)
struct Point {
    vec3 position;  // std430 aligns vec3 to 16 bytes
    vec3 color;     // std430 aligns vec3 to 16 bytes
    float intensity;
};

layout(std430, set = 0, binding = 1) readonly buffer PointBuffer {
    Point points[];
} pointData;

layout(location = 0) out vec3 fragColor;

const vec2 offsets[6] = vec2[](
    vec2(-1.0, -1.0), vec2( 1.0, -1.0), vec2( 1.0,  1.0),
    vec2(-1.0, -1.0), vec2( 1.0,  1.0), vec2(-1.0,  1.0)
);

void main() {
    Point pt = pointData.points[gl_InstanceIndex];
    vec3 centerPos = pt.position;

    // --- Render with True Colors ---
    // Using the RGB color loaded from the LAS file
    fragColor = pt.color;

    // Optional: If you want to switch back to Intensity:
    // fragColor = vec3(pt.intensity);

    // Quad Generation
    float pointSize = 0.025;
    vec2 offset = offsets[gl_VertexIndex] * pointSize;

    vec4 viewPos = ubo.view * ubo.model * vec4(centerPos, 1.0);
    viewPos.xy += offset;

    gl_Position = ubo.proj * viewPos;
}