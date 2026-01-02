#include "Application.hpp"
#include "BunnyLoader.hpp"


void Application::run() {

    std::vector<PositionVertex> bunnyVertices = loadBunny("assets/bunny/bun.conf", "assets/bunny/data/");

    if (bunnyVertices.empty()) {
        std::cerr << "Failed to load bunny model.\n";
    }

    while (!tgai.windowShouldClose(window)) {
        tgai.pollEvents(window);

        uint32_t currentFrame = tgai.nextFrame(window);

        tga::CommandRecorder recorder{tgai, commandBuffer};

        recorder.inlineBufferUpdate(uniformBuffer, &cameraData, sizeof(CameraData))
                .setRenderPass(renderPass, currentFrame, {0.1f, 0.1f, 0.1f, 1.0f}, 1.0f)
                .bindInputSet(inputSet)
                .draw(36, 0, 1, 0);

        commandBuffer = recorder.endRecording();

        tgai.execute(commandBuffer);
        tgai.present(window, currentFrame);
    }

    tgai.waitForCompletion(commandBuffer);
}