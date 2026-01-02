#include "Application.hpp"
#include "BunnyLoader.hpp"


void Application::run() {
    while (!tgai.windowShouldClose(window)) {
        tgai.pollEvents(window);

        uint32_t currentFrame = tgai.nextFrame(window);

        tga::CommandRecorder recorder{tgai, commandBuffer};

        recorder.inlineBufferUpdate(uniformBuffer, &cameraData, sizeof(CameraData))
                .setRenderPass(renderPass, currentFrame, {0.05f, 0.05f, 0.05f, 1.0f}, 1.0f)
                .bindInputSet(inputSet);
                // .draw(36, 0, 1, 0);

        bunnyModel->draw(recorder);

        commandBuffer = recorder.endRecording();

        tgai.execute(commandBuffer);
        tgai.present(window, currentFrame);
    }

    tgai.waitForCompletion(commandBuffer);
}