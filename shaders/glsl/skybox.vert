#version 450 core

layout(set = 0, binding = 0) uniform Camera {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 texCoords;

const vec3 skyboxVertices[8] = vec3[8](
    vec3(-1, -1, -1), vec3(-1, -1, 1), vec3(-1, 1, -1), vec3(-1, 1, 1),
    vec3(1, -1, -1), vec3(1, -1, 1), vec3(1, 1, -1), vec3(1, 1, 1)
);

const int skyboxFaces[36] = int[36](
    0, 1, 2,    1, 2, 3,    // Right
    4, 5, 6,    5, 6, 7,    // Left
    2, 3, 7,    2, 6, 7,    // Top
    0, 1, 5,    0, 4, 5,    // Bottom
    0, 2, 4,    2, 4, 6,    // Back
    1, 3, 5,    3, 5, 7     // Front
);

void main() {
    vec3 pos = skyboxVertices[skyboxFaces[gl_VertexIndex]];
    texCoords = pos;

    mat4 viewNoTranslation = mat4(mat3(ubo.view));
//    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
//    gl_Position = ubo.proj * viewNoTranslation * vec4(pos, 1.0);
    vec4 pos1 = ubo.proj * viewNoTranslation * vec4(pos, 1.0);
    gl_Position = pos1.xyww;
}