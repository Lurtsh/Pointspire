#pragma once
#ifndef POINTSPIRE_CAMERA_HPP
#define POINTSPIRE_CAMERA_HPP

#include <tga/tga.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camera {
    // State
    glm::vec3 pos{2.5f, 2.5f, 2.5f};
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

    // Euler Angles
    float yaw = -135.0f;
    float pitch = -35.0f;
    float fov = 60.0f;

    // Configuration
    const float moveSpeed = 2.5f;
    const float rotateSpeed = 90.0f; // Degrees per second
    const float zoomSpeed = 50.0f;

    void update(tga::Interface& tgai, tga::Window& window, float dt);

};

#endif //POINTSPIRE_CAMERA_HPP