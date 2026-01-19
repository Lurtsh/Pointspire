#include "Camera.hpp"
#include "tga/tga_math.hpp"

Camera::Camera(tga::Interface& _tgai) : tgai(_tgai) {
    tga::BufferInfo uboInfo{tga::BufferUsage::uniform, sizeof(CameraData)};
    uniformBuffer = tgai.createBuffer(uboInfo);
}

Camera::~Camera() {
    if (uniformBuffer) {
        tgai.free(uniformBuffer);
    }
}

tga::Buffer Camera::getUbo() const { return uniformBuffer; }

void Camera::update(tga::CommandRecorder& recorder, tga::Window& window, float dt) {
    // 1. Rotation (Arrows)
    float rotStep = rotateSpeed * dt;
    if (tgai.keyDown(window, tga::Key::Left)) yaw -= rotStep;
    if (tgai.keyDown(window, tga::Key::Right)) yaw += rotStep;
    if (tgai.keyDown(window, tga::Key::Up)) pitch += rotStep;
    if (tgai.keyDown(window, tga::Key::Down)) pitch -= rotStep;

    // Clamp Pitch
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Recalculate Basis Vectors
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));

    // 2. Movement (WASD)
    float moveStep = moveSpeed * dt;
    if (tgai.keyDown(window, tga::Key::W)) pos += front * moveStep;
    if (tgai.keyDown(window, tga::Key::S)) pos -= front * moveStep;
    if (tgai.keyDown(window, tga::Key::A)) pos -= right * moveStep;
    if (tgai.keyDown(window, tga::Key::D)) pos += right * moveStep;

    // 3. Zoom (Q/E)
    if (tgai.keyDown(window, tga::Key::Q)) fov -= zoomSpeed * dt;
    if (tgai.keyDown(window, tga::Key::E)) fov += zoomSpeed * dt;
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 120.0f) fov = 120.0f;

    // 4. Update UBO
    auto [width, height] = tgai.screenResolution();
    float aspect = static_cast<float>(width) / static_cast<float>(height);

    cameraData.model = glm::mat4(1.0f);
    cameraData.view = glm::lookAt(pos, pos + front, worldUp);
    cameraData.proj = glm::perspective_vk(glm::radians(fov), aspect, 0.1f, 1000.0f);

    // std::cout << "Cam pos is: " << pos.x << " " << pos.y << " " << pos.z << std::endl;
    
    recorder.inlineBufferUpdate(uniformBuffer, &cameraData, sizeof(CameraData));
}