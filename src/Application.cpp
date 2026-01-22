#include "Application.hpp"
#include <chrono>
#include <iostream>
#include <numeric>>

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

    // Create and build the Layered Point Cloud
    createLPCPipelines();
    buildLPC();
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




void Application::createLPCPipelines() {
    // 1. Morton
    // tga::Shader s_morton = loadCompShader("shaders/octree/1_morton.spv");
    tga::Shader mortonComputeShader = tga::loadShader("shaders/1_morton_comp.spv", tga::ShaderType::compute, tgai);
    tga::InputLayout l_morton{{
        {tga::BindingType::uniformBuffer}, {tga::BindingType::storageBuffer},
        {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer}
    }};
    m_lpcPasses.mortonPass = tgai.createComputePass({mortonComputeShader, l_morton});
    m_lpcInputSets.mortonSet = tgai.createInputSet({m_lpcPasses.mortonPass, {
        {pointCloud.getLPCUniformsBuffer(), 0}, {pointCloud.getSourceBuffer(), 1},
        {pointCloud.getMortonCodesBuffer(), 2}, {pointCloud.getSortIndicesBuffer(), 3}
    }});

    // 2. Bitonic
    tga::Shader bitonicSortComputeShader = tga::loadShader("shaders/2_bitonic_sort_comp.spv", tga::ShaderType::compute, tgai);
    tga::InputLayout l_bitonic{{
        {tga::BindingType::uniformBuffer}, {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer}, {tga::BindingType::uniformBuffer}
    }};
    m_lpcPasses.bitonicSortPass = tgai.createComputePass({bitonicSortComputeShader, l_bitonic});
    m_lpcInputSets.bitonicSortSet = tgai.createInputSet({m_lpcPasses.bitonicSortPass, {
        {pointCloud.getLPCUniformsBuffer(), 0}, {pointCloud.getMortonCodesBuffer(), 1},
        {pointCloud.getSortIndicesBuffer(), 2}, {pointCloud.getBitonicParamsBuffer(), 3}
    }});

    // 3. Reorder
    tga::Shader reorderComputeShader = tga::loadShader("shaders/3_reorder_comp.spv", tga::ShaderType::compute, tgai);
    tga::InputLayout l_reorder{{
        {tga::BindingType::uniformBuffer}, {tga::BindingType::storageBuffer},
        {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer}
    }};
    m_lpcPasses.reorderPass = tgai.createComputePass({reorderComputeShader, l_reorder});
    m_lpcInputSets.reorderSet = tgai.createInputSet({m_lpcPasses.reorderPass, {
        {pointCloud.getLPCUniformsBuffer(), 0}, {pointCloud.getSourceBuffer(), 1},
        {pointCloud.getSortIndicesBuffer(), 2}, {pointCloud.getVisibleBuffer(), 3} // Using Visible as temp dst
    }});

    // 4. Mark Heads
    tga::Shader markHeadsComputeShader = tga::loadShader("shaders/4_mark_heads_comp.spv", tga::ShaderType::compute, tgai);
    tga::InputLayout l_mark{{
        {tga::BindingType::uniformBuffer}, {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer}
    }};
    m_lpcPasses.markHeadsPass = tgai.createComputePass({markHeadsComputeShader, l_mark});
    m_lpcInputSets.markHeadsSet = tgai.createInputSet({m_lpcPasses.markHeadsPass, {
        {pointCloud.getLPCUniformsBuffer(), 0}, {pointCloud.getMortonCodesBuffer(), 1},
        {pointCloud.getHeadFlagsBuffer(), 2}
    }});

    // 5. Scatter
    tga::Shader scatterComputeShader = tga::loadShader("shaders/5_scatter_comp.spv", tga::ShaderType::compute, tgai);
    tga::InputLayout l_scatter{
        {
            {tga::BindingType::uniformBuffer}, {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer},
            {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer}
        }};
    m_lpcPasses.scatterPass = tgai.createComputePass({scatterComputeShader, l_scatter});
    m_lpcInputSets.scatterSet = tgai.createInputSet({m_lpcPasses.scatterPass, {
    {pointCloud.getLPCUniformsBuffer(), 0}, {pointCloud.getMortonCodesBuffer(), 1},
    {pointCloud.getHeadFlagsBuffer(), 2}, {pointCloud.getScannedIndicesBuffer(), 3},
    {pointCloud.getUniqueCodesBuffer(), 4}, {pointCloud.getVoxelStartsBuffer(), 5}}
    });

    // 6. Init Leaves
    tga::Shader initLeavesComputeShader = tga::loadShader("shaders/6_init_leaves_comp.spv", tga::ShaderType::compute, tgai);
    tga::InputLayout l_initLeaves{
        {
            {tga::BindingType::uniformBuffer}, {tga::BindingType::storageBuffer},
            {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer}
        }};
    m_lpcPasses.initLeavesPass = tgai.createComputePass({initLeavesComputeShader, l_initLeaves});
    m_lpcInputSets.initLeavesSet = tgai.createInputSet({m_lpcPasses.initLeavesPass, {
    {pointCloud.getLPCUniformsBuffer(), 0}, {pointCloud.getUniqueCodesBuffer(), 1},
        {pointCloud.getVoxelStartsBuffer(), 2}, {pointCloud.getNodesBuffer(), 3}
    }});

    tga::Shader buildInternalComputeShader = tga::loadShader("shaders/7_build_internal_comp.spv", tga::ShaderType::compute, tgai);
    tga::InputLayout l_buildInternal{
        {
            {tga::BindingType::uniformBuffer}, {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer},
        }};
    m_lpcPasses.buildInternalPass = tgai.createComputePass({buildInternalComputeShader, l_buildInternal});
    m_lpcInputSets.buildInternalSet = tgai.createInputSet({m_lpcPasses.buildInternalPass, {
    {pointCloud.getLPCUniformsBuffer(), 0}, {pointCloud.getUniqueCodesBuffer(), 1}, {pointCloud.getNodesBuffer(), 2}
    }});
}


void Application::buildLPC() {
    std::cout << "--- Building Layered Point Cloud ---" << std::endl;
    uint32_t numPoints = pointCloud.getTotalPointCount();
    auto dims = getDispatchDimensions(numPoints);

    // Temp Cmd Buffer for Phase 1
    tga::CommandBuffer cmd{};
    {


        tga::CommandRecorder rec(tgai, cmd);

        // 1. Morton
        std::cout << "- Computing Morton codes " << std::endl;
        rec.setComputePass(m_lpcPasses.mortonPass).bindInputSet(m_lpcInputSets.mortonSet);
        rec.dispatch(dims.first, dims.second, 1);
        rec.barrier(tga::PipelineStage::ComputeShader, tga::PipelineStage::VertexShader);

        // 2. Bitonic Sort
        std::cout << "- Sorting Morton codes" << std::endl;
        uint32_t pot = 1;
        while(pot < numPoints) pot <<= 1;

        struct SortParams { uint32_t j, k; };

        for (uint32_t k = 2; k <= pot; k <<= 1) {
            for (uint32_t j = k >> 1; j > 0; j >>= 1) {
                SortParams p{j, k};

                // A. Update the UBO with current stage parameters
                rec.inlineBufferUpdate(pointCloud.getBitonicParamsBuffer(), &p, sizeof(p));

                // B. Barrier: Ensure Transfer (Update) completes before Compute reads UBO
                rec.barrier(tga::PipelineStage::Transfer, tga::PipelineStage::ComputeShader);

                // C. Dispatch Sort Step
                rec.setComputePass(m_lpcPasses.bitonicSortPass).bindInputSet(m_lpcInputSets.bitonicSortSet);
                rec.dispatch(dims.first, dims.second, 1);

                // D. Barrier:
                // 1. Compute -> Compute: Ensure sorting of this step finishes before next step reads data
                // 2. Compute -> Transfer: Ensure shader is done reading 'j,k' before we overwrite them in next loop
                rec.barrier(tga::PipelineStage::ComputeShader,
                            tga::PipelineStage::ComputeShader);
            }
        }

        // 3. Reorder
        std::cout << "- Reordering Morton codes" << std::endl;
        rec.setComputePass(m_lpcPasses.reorderPass).bindInputSet(m_lpcInputSets.reorderSet);
        rec.dispatch(dims.first, dims.second, 1);
        rec.barrier(tga::PipelineStage::ComputeShader, tga::PipelineStage::ComputeShader);

        // 4. Mark Heads
        std::cout << "- Marking Heads" << std::endl;
        rec.setComputePass(m_lpcPasses.markHeadsPass).bindInputSet(m_lpcInputSets.markHeadsSet);
        rec.dispatch(dims.first, dims.second, 1);
        rec.barrier(tga::PipelineStage::ComputeShader, tga::PipelineStage::Transfer); // Ready for download

        cmd = rec.endRecording();
        tgai.execute(cmd);
        tgai.waitForCompletion(cmd);
        // tgai.free(cmd);
    }

    std::cout << "Tracing: Stage 1 is complete" << std::endl;
    // --- PHASE 2: CPU SCAN ---
    // Download Flags
    std::vector<uint32_t> flags(numPoints);
    tga::StagingBuffer stageFlags = tgai.createStagingBuffer({numPoints * sizeof(uint32_t)});

    std::cout << "Tracing: Created host-side staging buffer" << std::endl;
    // Command to download
    {
    // tga::CommandBuffer cmd2{};

        tga::CommandRecorder rec(tgai);
        rec.bufferDownload(pointCloud.getHeadFlagsBuffer(), stageFlags, numPoints * sizeof(uint32_t));
        // tgai.endCommandBuffer(cmd);
        rec.endRecording();

        tgai.execute(cmd);
        tgai.waitForCompletion(cmd);
        // tgai.free(cmd);
        std::cout << "Tracing: waiting for buffer download! " << std::endl;
    }

    // Map and Read
    uint8_t* ptr = static_cast<uint8_t*>(tgai.getMapping(stageFlags));
    std::memcpy(flags.data(), ptr, flags.size() * sizeof(uint32_t));

    // Exclusive Scan
    std::vector<uint32_t> scanned(numPoints);
    std::exclusive_scan(flags.begin(), flags.end(), scanned.begin(), 0);
    uint32_t numUnique = scanned.back() + flags.back();

    // Update Uniforms with NumUnique
    LPCUniforms u = {pointCloud.getBounds(), numPoints, numUnique};

    // Upload Scanned + Uniforms
    tga::StagingBuffer stageScan = tgai.createStagingBuffer({numPoints * sizeof(uint32_t), reinterpret_cast<uint8_t*>(scanned.data())});
    tga::StagingBuffer stageUniform = tgai.createStagingBuffer({sizeof(u), reinterpret_cast<uint8_t*>(&u)});

    std::cout << "Tracing: Stage 2 (CPU) is complete" << std::endl;
    // --- PHASE 3: GPU AGAIN ---

    // cmd = tga::CommandBuffer{};
    {
        tga::CommandRecorder rec(tgai);
        rec.bufferUpload(stageScan, pointCloud.getScannedIndicesBuffer(), numPoints * sizeof(uint32_t));
        rec.bufferUpload(stageUniform, pointCloud.getLPCUniformsBuffer(), sizeof(u));
        rec.barrier(tga::PipelineStage::Transfer, tga::PipelineStage::ComputeShader);

        // 5. Scatter
        rec.setComputePass(m_lpcPasses.scatterPass).bindInputSet(m_lpcInputSets.scatterSet);
        rec.dispatch(dims.first, dims.second, 1);
        rec.barrier(tga::PipelineStage::ComputeShader, tga::PipelineStage::ComputeShader);

        // 6. Init Leaves
        rec.setComputePass(m_lpcPasses.initLeavesPass).bindInputSet(m_lpcInputSets.initLeavesSet);
        rec.dispatch(dims.first, dims.second, 1);
        rec.barrier(tga::PipelineStage::ComputeShader, tga::PipelineStage::ComputeShader);

        // 7. Build Internal
        rec.setComputePass(m_lpcPasses.buildInternalPass).bindInputSet(m_lpcInputSets.buildInternalSet);
        rec.dispatch(dims.first, dims.second, 1);
        rec.barrier(tga::PipelineStage::ComputeShader, tga::PipelineStage::VertexShader);

        rec.endRecording();
        tgai.execute(cmd);
        tgai.waitForCompletion(cmd);
        tgai.free(cmd);
    }

     std::cout << "FINISHED!" << std::endl;

}