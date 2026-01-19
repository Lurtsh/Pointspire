#pragma once
#ifndef POINTSPIRE_CAMERA_HPP
#define POINTSPIRE_CAMERA_HPP

#include <tga/tga.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Forward declare to avoid including the header
namespace tga {
    struct Interface;
    struct Window;
    struct CommandRecorder;
}

class Camera {
public:
    Camera(tga::Interface& tgai);
    ~Camera();

    // Prevent copying
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    void update(tga::CommandRecorder& recorder, tga::Window& window, float dt);
    tga::Buffer getUbo() const;

private:
    tga::Interface& tgai;

    struct CameraData {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };
    CameraData cameraData;
    tga::Buffer uniformBuffer;

    // State
    glm::vec3 pos{100.0f, 100.0f, 100.0f};//{631680, 5.26862e+06, 950.0f};
    glm::vec3 front{0.0f, 0.0f, 1.0f};
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

    // Euler Angles
    float yaw = -135.0f;
    float pitch = 0.0f;
    float fov = 60.0f;

    // Configuration
    const float moveSpeed = 25.0f;
    const float rotateSpeed = 90.0f; // Degrees per second
    const float zoomSpeed = 50.0f;
};

#endif //POINTSPIRE_CAMERA_HPP