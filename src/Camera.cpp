#include "Camera.hpp"

void Camera::update(tga::Interface& tgai, tga::Window& window, float dt) {
    // 1. Rotation (Arrows)
    float rotStep = rotateSpeed * dt;

    if (tgai.keyDown(window, tga::Key::Left))  yaw -= rotStep;
    if (tgai.keyDown(window, tga::Key::Right)) yaw += rotStep;
    if (tgai.keyDown(window, tga::Key::Up))    pitch += rotStep;
    if (tgai.keyDown(window, tga::Key::Down))  pitch -= rotStep;

    // Clamp Pitch (avoid gimbal lock/flip)
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

    // 2. Movement (WASD) - Relative to View
    float moveStep = moveSpeed * dt;

    if (tgai.keyDown(window, tga::Key::W)) pos += front * moveStep;
    if (tgai.keyDown(window, tga::Key::S)) pos -= front * moveStep;
    if (tgai.keyDown(window, tga::Key::A)) pos -= right * moveStep;
    if (tgai.keyDown(window, tga::Key::D)) pos += right * moveStep;

    // 3. Zoom (Q/E)
    if (tgai.keyDown(window, tga::Key::Q)) fov -= zoomSpeed * dt;
    if (tgai.keyDown(window, tga::Key::E)) fov += zoomSpeed * dt;

    // Clamp FOV
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 120.0f) fov = 120.0f;
}