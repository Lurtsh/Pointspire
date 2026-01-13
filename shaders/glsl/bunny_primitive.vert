// #version 450

// // Binding 0: Camera Data
// layout(set = 0, binding = 0) uniform Camera {
//     mat4 model;
//     mat4 view;
//     mat4 proj;
// } ubo;

// // Binding 1: Point Cloud Data (SSBO)
// struct PositionVertex {
//     vec3 position;
// };

// layout(std430, set = 0, binding = 1) readonly buffer PointBuffer {
//     PositionVertex vertices[];
// } pointData;

// layout(location = 0) out vec3 fragColor;


// const vec2 offsets[6] = vec2[](
// vec2(-1.0, -1.0),
// vec2( 1.0, -1.0),
// vec2( 1.0,  1.0),
// vec2(-1.0, -1.0),
// vec2( 1.0,  1.0),
// vec2(-1.0,  1.0)
// );

// void main() {
//     vec3 centerPos = pointData.vertices[gl_InstanceIndex].position;

//     float pointSize = 0.1; // Adjust this scale for larger/smaller points
//     vec2 offset = offsets[gl_VertexIndex] * pointSize;

//     vec4 viewPos = ubo.view * ubo.model * vec4(centerPos, 1.0);

//     viewPos.xy += offset;

//     gl_Position = ubo.proj * viewPos;

//     fragColor = vec3(1.0, 0.0, 0.0);
// }

// #version 450

// // Binding 0: Scene Data
// // In C++, struct SceneData { mat4 model; mat4 view; mat4 proj; float maxHeight; int colorMode; vec2 padding; };
// layout(set = 0, binding = 0) uniform SceneData {
//     mat4 model;
//     mat4 view;
//     mat4 proj;
//     float maxHeight; // Used for height map normalization
//     int colorMode;   // 0 = Height Map, 1 = Real Color (RGB), 2 = Intensity
// } ubo;

// // Binding 1: Point Cloud Data (SSBO)
// struct Point {
//     vec3 position;  // Base alignment 16N -> 16 bytes. Implicit padding of 4 bytes follows.
//     vec3 color;     // Starts at offset 16.
//     float intensity;// Starts at offset 28.
// };

// layout(std430, set = 0, binding = 1) readonly buffer PointBuffer {
//     Point points[];
// } pointData;
// layout(location = 0) out vec3 fragColor;

// // Quad generation offsets
// const vec2 offsets[6] = vec2[](
//     vec2(-1.0, -1.0), vec2( 1.0, -1.0), vec2( 1.0,  1.0),
//     vec2(-1.0, -1.0), vec2( 1.0,  1.0), vec2(-1.0,  1.0)
// );

// // Function to get color from height (Geography style)
// vec3 getHeightColor(float height, float maxH) {
//     float t = clamp(height / maxH, 0.0, 1.0);
    
//     vec3 low = vec3(0.0, 0.0, 0.5);   // Deep Blue
//     vec3 mid = vec3(0.2, 0.6, 0.2);   // Green
//     vec3 high = vec3(0.9, 0.9, 0.9);  // White/Snow
    
//     if (t < 0.5) {
//         return mix(low, mid, t * 2.0);
//     } else {
//         return mix(mid, high, (t - 0.5) * 2.0);
//     }
// }

// void main() {
//     // 1. Fetch Point Data using Instance Index
//     Point pt = pointData.points[gl_InstanceIndex];
//     vec3 centerPos = pt.position;
    
//     // 2. Determine Color based on Mode
//     if (ubo.colorMode == 1) {
//         // Real RGB from LAS
//         fragColor = pt.color;
//     } 
//     else if (ubo.colorMode == 2) {
//         // Intensity visualization (Grayscale)
//         fragColor = vec3(pt.intensity);
//     }
//     else {
//         // Default: Height Map
//         // Assuming Y is Up. If Z is up in your view matrix, use centerPos.z
//         fragColor = getHeightColor(centerPos.y, ubo.maxHeight);
//     }

//     // 3. Procedural Point Size
//     float pointSize = 0.05; 
//     vec2 offset = offsets[gl_VertexIndex] * pointSize;

//     // Billboard calculation (simple view-plane aligned)
//     vec4 viewPos = ubo.view * ubo.model * vec4(centerPos, 1.0);
//     viewPos.xy += offset;
    
//     gl_Position = ubo.proj * viewPos;
// }

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
    float pointSize = 0.05; 
    vec2 offset = offsets[gl_VertexIndex] * pointSize;

    vec4 viewPos = ubo.view * ubo.model * vec4(centerPos, 1.0);
    viewPos.xy += offset;
    
    gl_Position = ubo.proj * viewPos;
}