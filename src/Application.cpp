#include "Application.hpp"
#include "BunnyLoader.hpp"

#include <chrono>
//#include "Overlay.hpp"

void Application::run() {
    while (!tgai.windowShouldClose(window)) {
        tgai.pollEvents(window);

        uint32_t currentFrame = tgai.nextFrame(window);

        tga::CommandRecorder recorder{tgai, commandBuffer};

        recorder.inlineBufferUpdate(uniformBuffer, &cameraData, sizeof(CameraData))
                .setRenderPass(renderPass, currentFrame, {0.05f, 0.05f, 0.05f, 1.0f}, 1.0f)
                .bindInputSet(inputSet)
                .draw(36, 0, 1, 0);

        pointCloud.draw(recorder);

        commandBuffer = recorder.endRecording();

        tgai.execute(commandBuffer);
        tgai.present(window, currentFrame);


    }

    tgai.waitForCompletion(commandBuffer);
}

// void Application::run() {
//     auto [scrW, scrH] = tgai.screenResolution();
//
//     // Time management
//     auto lastTime = std::chrono::high_resolution_clock::now();
//
//     while (!tgai.windowShouldClose(window)) {
//         // Calculate Delta Time
//         auto currentTime = std::chrono::high_resolution_clock::now();
//         float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
//         lastTime = currentTime;
//
//         tgai.pollEvents(window);
//         uint32_t currentFrame = tgai.nextFrame(window);
//
//         // Update Camera
//         camera.update(tgai, window, dt);
//
//         // Update Uniforms
//         float aspect = static_cast<float>(scrW) / static_cast<float>(scrH);
//         // Note: Assuming window size approx half screen, using screen aspect for simplicity
//         // or if window size retrieval was available we'd use that.
//
//         CameraData cameraData;
//         cameraData.model = glm::mat4(1.0f);
//         // cameraData.view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.worldUp);
//         cameraData.view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.worldUp);
//         cameraData.proj = glm::perspective_vk(glm::radians(camera.fov), aspect, 0.1f, 100.0f);
//
//         // Record and Draw
//         // tga::CommandBuffer{} is implicit if commandBuffer is empty initially
//         tga::CommandRecorder recorder{tgai, commandBuffer};
//
//         recorder.inlineBufferUpdate(uniformBuffer, &cameraData, sizeof(CameraData))
//                 .setRenderPass(renderPass, currentFrame, {0.1f, 0.1f, 0.1f, 1.0f}, 1.0f)
//                 .bindInputSet(inputSet)
//                 .draw(36, 0, 1, 0);
//
//         // Update the handle for the next frame
//         commandBuffer = recorder.endRecording();
//
//         tgai.execute(commandBuffer);
//         tgai.present(window, currentFrame);
//     }
//
//     tgai.waitForCompletion(commandBuffer);
// }