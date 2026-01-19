#include "Application.hpp"
#include <chrono>
#include <iostream>

Application::Application(tga::Interface& _tgai)
    : tgai(_tgai), pointCloud(tgai), camera(tgai), scene(tgai)
{
    auto [scrW, scrH] = tgai.screenResolution();
    // Using a sensible window size
    tga::WindowInfo winInfo{1600, 900, tga::PresentMode::vsync};
    window = tgai.createWindow(winInfo);
    tgai.setWindowTitle(window, "Pointspire");

    // =========================================================
    // 1. Configure Skybox Pipeline (Background)
    // =========================================================
    skyVertShader = tga::loadShader("shaders/skybox_vert.spv", tga::ShaderType::vertex, tgai);
    skyFragShader = tga::loadShader("shaders/skybox_frag.spv", tga::ShaderType::fragment, tgai);

    tga::InputLayout skyLayout{{
        {tga::BindingType::uniformBuffer, 1}, // Camera
        {tga::BindingType::sampler, 1}        // CubeMap
    }};

    // Skybox is drawn first and handles the screen clear.
    tga::RenderPassInfo skyPassInfo{
        skyVertShader, skyFragShader, window, {},
        skyLayout,
        tga::ClearOperation::all, // IMPORTANT: Clears Color and Depth buffers
        tga::PerPixelOperations{tga::CompareOperation::lessEqual, false}, // Standard depth test
        tga::RasterizerConfig{tga::FrontFace::counterclockwise, tga::CullMode::none}
    };
    skyRenderPass = tgai.createRenderPass(skyPassInfo);

    tga::InputSetInfo skySetInfo{
        skyRenderPass,
        { {camera.getUbo(), 0, 0}, {scene.getSkyCubemap(), 1, 0} },
        0
    };
    skyInputSet = tgai.createInputSet(skySetInfo);

    // =========================================================
    // 2. Configure Point Cloud Pipeline (Geometry)
    // =========================================================
    pcVertShader = tga::loadShader("shaders/bunny_primitive_vert.spv", tga::ShaderType::vertex, tgai);
    pcFragShader = tga::loadShader("shaders/bunny_primitive_frag.spv", tga::ShaderType::fragment, tgai);

    tga::InputLayout pcLayout{{
        {tga::BindingType::uniformBuffer}, // Camera
        {tga::BindingType::storageBuffer}, // Points
    }};

    // Point Cloud is drawn second. It MUST NOT clear the screen, or the skybox is lost.
    tga::RenderPassInfo pcPassInfo{
        pcVertShader, pcFragShader, window, {},
        pcLayout,
        tga::ClearOperation::none, // IMPORTANT: Loads existing Color/Depth from Skybox pass
        tga::PerPixelOperations{tga::CompareOperation::less, false}, // Write depth
        tga::RasterizerConfig{tga::FrontFace::counterclockwise, tga::CullMode::back}
    };
    pcRenderPass = tgai.createRenderPass(pcPassInfo);

    tga::InputSetInfo pcSetInfo{
        pcRenderPass,
        { {camera.getUbo(), 0, 0}, {pointCloud.getVisibleBuffer(), 1, 0} },
        0
    };
    pcInputSet = tgai.createInputSet(pcSetInfo);

    commandBuffer = tga::CommandBuffer{};

    // =========================================================
    // 3. Configure Frustum Culling Compute Pipeline
    // =========================================================

    cullingShader = tga::loadShader("shaders/cull_comp.spv", tga::ShaderType::compute, tgai);

    // Layout matches shader bindings:
    // 0: Camera UBO (MVP matrices)
    // 1: Source SSBO (All points)
    // 2: Destination SSBO (Visible points)
    // 3: Indirect Buffer (Draw command)
    // 4: Cull Info UBO (Total point count)
    tga::InputLayout cullLayout{{
        {tga::BindingType::uniformBuffer},
        {tga::BindingType::storageBuffer},
        {tga::BindingType::storageBuffer},
        {tga::BindingType::storageBuffer},
        {tga::BindingType::uniformBuffer}
    }};

    tga::ComputePassInfo info{cullingShader, cullLayout};
    cullPass = tgai.createComputePass(info);

    tga::InputSetInfo setInfo{
        cullPass,
        {
            {camera.getUbo(), 0, 0},
            {pointCloud.getSourceBuffer(), 1, 0},
            {pointCloud.getVisibleBuffer(), 2, 0},
            {pointCloud.getIndirectBuffer(), 3, 0},
            {pointCloud.getCullInfoUBO(), 4, 0}
        },
        0
    };

    cullInputSet = tgai.createInputSet(setInfo);
}

Application::~Application() {
    // Free Compute Resources
    if (cullInputSet) tgai.free(cullInputSet);
    if (cullPass) tgai.free(cullPass);
    if (cullingShader) tgai.free(cullingShader);

    // Free Point Cloud Resources
    if (pcInputSet) tgai.free(pcInputSet);
    if (pcRenderPass) tgai.free(pcRenderPass);
    if (pcVertShader) tgai.free(pcVertShader);
    if (pcFragShader) tgai.free(pcFragShader);

    // Free Skybox Resources
    if (skyInputSet) tgai.free(skyInputSet);
    if (skyRenderPass) tgai.free(skyRenderPass);
    if (skyVertShader) tgai.free(skyVertShader);
    if (skyFragShader) tgai.free(skyFragShader);

    if (commandBuffer) tgai.free(commandBuffer);
    if (window) tgai.free(window);
}

void Application::run() {
    auto lastTime = std::chrono::high_resolution_clock::now();

    if (!cullPass) {
        std::cerr << "Failed to initialize the cull pass" << std::endl;
        return;
    }

    while (!tgai.windowShouldClose(window)) {
        // --- Time and Polling logic ---
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        lastTime = currentTime;

        tgai.pollEvents(window);
        uint32_t currentFrame = tgai.nextFrame(window);

        tga::CommandRecorder recorder{tgai, commandBuffer};

        // 1. Update Camera
        camera.update(recorder, window, dt);

        // 2. COMPUTE CULLING
        // Reset the instance count in the indirect buffer to 0.
        // The compute shader will atomically increment this for every visible point.
        uint32_t resetCount = 0;
        recorder.inlineBufferUpdate(pointCloud.getIndirectBuffer(), &resetCount, sizeof(uint32_t), offsetof(tga::DrawIndirectCommand, instanceCount));

        // Barrier: Ensure the buffer update finishes before the Compute Shader reads/writes it.
        recorder.barrier(tga::PipelineStage::Transfer, tga::PipelineStage::ComputeShader);

        recorder.setComputePass(cullPass).bindInputSet(cullInputSet);

        // Dispatch Compute Shader
        // Use helper to handle large point counts that exceed hardware limit (65535) on X-axis.
        auto [groupSizeX, groupSizeY] = getDispatchDimensions(pointCloud.getTotalPointCount());
        recorder.dispatch(groupSizeX, groupSizeY, 1);

        // Barrier: Ensure Compute finishes writing point data and instance count
        // before the Vertex Shader (draw) and Indirect Command Processor try to use them.
        recorder.barrier(tga::PipelineStage::ComputeShader, tga::PipelineStage::VertexShader);


        // 3. DRAW SKYBOX (Background)
        // This pass clears the color/depth attachments.
        recorder.setRenderPass(skyRenderPass, currentFrame)
                .bindInputSet(skyInputSet)
                .drawIndirect(scene.getIndirectBuffer(), 1, 0, sizeof(tga::DrawIndirectCommand));

        // 4. DRAW POINT CLOUD (Geometry)
        // This pass Loads attachments. It uses the buffer filled by the Compute Shader step.
        recorder.setRenderPass(pcRenderPass, currentFrame)
                .bindInputSet(pcInputSet)
                .drawIndirect(pointCloud.getIndirectBuffer(), 1, 0, sizeof(tga::DrawIndirectCommand));

        commandBuffer = recorder.endRecording();
        tgai.execute(commandBuffer);
        tgai.present(window, currentFrame);
    }
    tgai.waitForCompletion(commandBuffer);
}

std::pair<uint32_t, uint32_t> Application::getDispatchDimensions(size_t numThreads, uint32_t workGroupSize) {
    if (numThreads == 0) return {0, 0};

    // Standard Vulkan/OpenGL hardware limit for dimension X
    const uint32_t MAX_DIM_X = 65535;

    // 1. Calculate total workgroups needed (Ceiling Division)
    // using uint64_t to prevent overflow during calculation before division
    uint64_t totalGroups = (numThreads + workGroupSize - 1) / workGroupSize;

    // 2. If it fits in one dimension, return (Total, 1)
    if (totalGroups <= MAX_DIM_X) {
        return {static_cast<uint32_t>(totalGroups), 1};
    }

    // 3. Otherwise, fill X completely and spill the remainder to Y
    uint32_t groupCountX = MAX_DIM_X;
    auto groupCountY = static_cast<uint32_t>((totalGroups + MAX_DIM_X - 1) / MAX_DIM_X);

    return {groupCountX, groupCountY};
}