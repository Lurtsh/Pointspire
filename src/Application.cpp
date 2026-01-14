// #include "Application.hpp"
// #include "BunnyLoader.hpp"
//
// #include <chrono>
// //#include "Overlay.hpp"
//
// void Application::run() {
//     while (!tgai.windowShouldClose(window)) {
//         tgai.pollEvents(window);
//
//         uint32_t currentFrame = tgai.nextFrame(window);
//
//         tga::CommandRecorder recorder{tgai, commandBuffer};
//
//         recorder.inlineBufferUpdate(uniformBuffer, &cameraData, sizeof(CameraData))
//                 .setRenderPass(renderPass, currentFrame, {0.05f, 0.05f, 0.05f, 1.0f}, 1.0f)
//                 .bindInputSet(inputSet)
//                 .draw(36, 0, 1, 0);
//
//         pointCloud.draw(recorder);
//
//         commandBuffer = recorder.endRecording();
//
//         tgai.execute(commandBuffer);
//         tgai.present(window, currentFrame);
//
//
//     }
//
//     tgai.waitForCompletion(commandBuffer);
// }

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

#include "Application.hpp"
#include <chrono>

// Helper to keep code clean
struct SceneData {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    float maxHeight;
    int colorMode;
};

void Application::run() {
    auto [scrW, scrH] = tgai.screenResolution();

    // 1. Setup Camera UBO
    tga::BufferInfo uboInfo{tga::BufferUsage::uniform, sizeof(SceneData)};
    uniformBuffer = tgai.createBuffer(uboInfo);

    // 2. Initialize Scene Pipelines
    // Pass the UBO to Scene so it can build the skybox InputSet
    scene.get()->initSkyboxPipeline(window, uniformBuffer);

    // Initialize PointCloud RenderPass (assuming PointCloud class needs one passed or creates internally)
    // For this example, let's assume we create the PC pass here to control the ClearOp

    // Point Cloud Pass: CLEAR EVERYTHING
    tga::InputLayout pcLayout{{
        {tga::BindingType::uniformBuffer, 1},
        {tga::BindingType::storageBuffer, 1}
    }};

    tga::RenderPassInfo pcPassInfo{
        vertShader, fragShader, window, {}, pcLayout,
        tga::ClearOperation::all, // <--- Clears screen
        tga::PerPixelOperations{tga::CompareOperation::less, false},
        tga::RasterizerConfig{tga::FrontFace::counterclockwise, tga::CullMode::back}
    };
    tga::RenderPass pointCloudPass = tgai.createRenderPass(pcPassInfo);

    // Create PC Input Set (assuming PointCloud class has a method or we do it here)
    tga::InputSet pcInputSet = tgai.createInputSet({
        pointCloudPass,
        {{uniformBuffer, 0, 0}, {pointCloud.getBuffer(), 1, 0}},
        0
    });

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!tgai.windowShouldClose(window)) {
        // --- Logic ---
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        lastTime = currentTime;

        tgai.pollEvents(window);
        uint32_t currentFrame = tgai.nextFrame(window);

        // Update Camera
        camera.update(tgai, window, dt);

        float aspect = static_cast<float>(scrW) / static_cast<float>(scrH);
        SceneData uboData;
        uboData.model = glm::mat4(1.0f);
        uboData.view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.worldUp);
        uboData.proj = glm::perspective_vk(glm::radians(camera.fov), aspect, 0.1f, 1000.0f);
        uboData.maxHeight = pointCloud.getMaxHeight();
        uboData.colorMode = 1; // True Color

        // --- Rendering ---
        tga::CommandRecorder recorder{tgai, commandBuffer};

        // Update Uniforms (Inline is fast for small data)
        recorder.inlineBufferUpdate(uniformBuffer, &uboData, sizeof(SceneData));

        // PASS 1: Point Cloud (Opaque)
        // This clears the screen and writes depth.
        recorder.setRenderPass(pointCloudPass, currentFrame, {0.05f, 0.05f, 0.05f, 1.0f}, 1.0f)
                .bindInputSet(pcInputSet);

        // PointCloud uses its own indirect buffer internally
        pointCloud.draw(recorder);

        // PASS 2: Skybox (Background)
        // This loads the previous result and only draws at max depth (z=1.0)
        // thanks to CompareOperation::lessEqual in Scene::initSkyboxPipeline
        scene.get()->drawSkybox(recorder, currentFrame);

        commandBuffer = recorder.endRecording();
        tgai.execute(commandBuffer);
        tgai.present(window, currentFrame);
    }

    tgai.waitForCompletion(commandBuffer);
    tgai.free(pointCloudPass);
    tgai.free(pcInputSet);
    tgai.free(uniformBuffer);
}