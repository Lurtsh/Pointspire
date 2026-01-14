// #include "Scene.hpp"
//
// Scene::Scene(tga::Interface &tgai) : tgai(tgai) {
//     std::vector<std::string> faces = {
//         "assets/textures/skybox/right.jpg",
//         "assets/textures/skybox/left.jpg",
//         "assets/textures/skybox/top.jpg",
//         "assets/textures/skybox/bottom.jpg",
//         "assets/textures/skybox/front.jpg",
//         "assets/textures/skybox/back.jpg",
//     };
//
//     loadSkyCubemap(faces);
// }
//
// void Scene::loadSkyCubemap(const std::string filepath) {
//     // skyCubemap = tga::loadTexture(filepath, tga::Format::r8g8b8a8_unorm, tga::SamplerMode::linear, tga::AddressMode::clampEdge, tgai);
//     // stbi_load()
//
//     auto loadTex = [&](tga::Texture& tex, std::string const& filepath, tga::Format format) {
//         if (filepath.size() > 0) {
//             int width, height, channels;
//             stbi_uc *pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
//             if (pixels) {
//                 auto textureStagingBuffer = tgai.createStagingBuffer({width * height * 4, pixels});
//                 tex = tgai.createTexture(tga::TextureInfo(
//                     static_cast<uint32_t>(width),
//                     static_cast<uint32_t>(height), format,
//                     tga::SamplerMode::nearest, tga::AddressMode::clampEdge,
//                     tga::TextureType::_Cube, 6, textureStagingBuffer, 0));
//                 stbi_image_free(pixels);
//             }
//         }
//     };
//     loadTex(skyCubemap.texture, filepath, tga::Format::r8g8b8a8_srgb);
// }
//
// void Scene::loadSkyCubemap(std::vector<std::string> filepaths) {
//     std::vector<uint8_t> cubeData;
//     int width, height, channels;
//     uint32_t faceWidth = 0;
//     uint32_t faceHeight = 0;
//
//     for (size_t i = 0; i < filepaths.size(); ++i) {
//         stbi_uc *pixels = stbi_load(filepaths[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
//
//         if (!pixels) {
//             std::cerr << "Failed to load skybox face: " << filepaths[i] << std::endl;
//             return;
//         }
//
//         if (i == 0) {
//             faceWidth = static_cast<uint32_t>(width);
//             faceHeight = static_cast<uint32_t>(height);
//
//             cubeData.reserve(6 * faceWidth * faceHeight);
//         } else {
//             if (static_cast<uint32_t>(width) != faceWidth || static_cast<uint32_t>(height) != faceHeight) {
//                 std::cerr << "Error: Skybox face dimensions mismatch at " << filepaths[i] << std::endl;
//                 stbi_image_free(pixels);
//                 return;
//             }
//         }
//
//         size_t faceSize = width * height * 4;
//         cubeData.insert(cubeData.end(), pixels, pixels + faceSize);
//
//         stbi_image_free(pixels);
//     }
//
//     auto stagingBuffer = tgai.createStagingBuffer({cubeData.size(), cubeData.data()});
//
//     skyCubemap.texture = tgai.createTexture(tga::TextureInfo(
//         faceWidth,
//         faceHeight,
//         tga::Format::r8g8b8a8_srgb,
//         tga::SamplerMode::nearest,
//         tga::AddressMode::clampEdge,
//         tga::TextureType::_Cube,
//         6,
//         stagingBuffer,
//         0));
//
//     skyCubemap.width = faceWidth;
//     skyCubemap.height = faceHeight;
// }
//

#include "Scene.hpp"
#include <iostream>

Scene::Scene(tga::Interface &tgai) : tgai(tgai) {
    // 1. Load Textures
    std::vector<std::string> faces = {
        "assets/textures/skybox/right.jpg", "assets/textures/skybox/left.jpg",
        "assets/textures/skybox/top.jpg",   "assets/textures/skybox/bottom.jpg",
        "assets/textures/skybox/front.jpg", "assets/textures/skybox/back.jpg",
    };
    loadSkyCubemap(faces);

    // 2. Create Indirect Buffer for Skybox (36 vertices, 1 instance)
    tga::DrawIndirectCommand cmd{36, 1, 0, 0};
    tga::StagingBufferInfo cmdStaging{sizeof(cmd), reinterpret_cast<uint8_t*>(&cmd)};

    m_indirectBuffer = tgai.createBuffer({
        tga::BufferUsage::indirect | tga::BufferUsage::storage,
        sizeof(cmd),
        tgai.createStagingBuffer(cmdStaging)
    });
}

Scene::~Scene() {
    if (m_vertShader) tgai.free(m_vertShader);
    if (m_fragShader) tgai.free(m_fragShader);
    if (m_renderPass) tgai.free(m_renderPass);
    if (m_inputSet) tgai.free(m_inputSet);
    if (m_indirectBuffer) tgai.free(m_indirectBuffer);
    if (skyCubemap.texture) tgai.free(skyCubemap.texture);
}

void Scene::initSkyboxPipeline(tga::Window window, tga::Buffer cameraUBO) {
    // Load Shaders
    m_vertShader = tga::loadShader("shaders/skybox_vert.spv", tga::ShaderType::vertex, tgai);
    m_fragShader = tga::loadShader("shaders/skybox_frag.spv", tga::ShaderType::fragment, tgai);

    // Input Layout: Binding 0 = Camera UBO, Binding 1 = Cubemap
    tga::InputLayout inputLayout{{
        {tga::BindingType::uniformBuffer, 1},
        {tga::BindingType::sampler, 1}
    }};

    // Render Pass Configuration
    // OPTIMIZATION: ClearOperation::none -> We load the result of the PointCloud pass
    // OPTIMIZATION: CompareOperation::lessEqual -> Skybox is at z=1.0, draws only if background
    tga::RenderPassInfo passInfo{
        m_vertShader, m_fragShader, window, {},
        inputLayout,
        tga::ClearOperation::none,
        tga::PerPixelOperations{tga::CompareOperation::lessEqual, false},
        tga::RasterizerConfig{tga::FrontFace::counterclockwise, tga::CullMode::none}
    };
    m_renderPass = tgai.createRenderPass(passInfo);

    // Input Set
    tga::InputSetInfo setInfo{
        m_renderPass,
        {
            {cameraUBO, 0, 0},
            {skyCubemap.texture, 1, 0}
        },
        0
    };
    m_inputSet = tgai.createInputSet(setInfo);
}

void Scene::drawSkybox(tga::CommandRecorder& recorder, uint32_t frameIndex) {
    if (!m_renderPass) return;

    // We do NOT clear here (ClearOperation::none was set in init)
    // Preserves the Point Cloud drawn previously
    recorder.setRenderPass(m_renderPass, frameIndex)
            .bindInputSet(m_inputSet)
            .drawIndirect(m_indirectBuffer, 1, 0, sizeof(tga::DrawIndirectCommand));
}

void Scene::loadSkyCubemap(const std::vector<std::string>& filepaths) {
    // ... [Same implementation as your provided code] ...
    // (Ensure you copy the exact loadSkyCubemap implementation provided in your prompt)
    // ...
    std::vector<uint8_t> cubeData;
    int width, height, channels;
    uint32_t faceWidth = 0;
    uint32_t faceHeight = 0;

    for (size_t i = 0; i < filepaths.size(); ++i) {
        stbi_uc *pixels = stbi_load(filepaths[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels) return;

        if (i == 0) {
            faceWidth = width; faceHeight = height;
            cubeData.reserve(6 * faceWidth * faceHeight * 4);
        }

        size_t faceSize = width * height * 4;
        cubeData.insert(cubeData.end(), pixels, pixels + faceSize);
        stbi_image_free(pixels);
    }

    auto staging = tgai.createStagingBuffer({cubeData.size(), cubeData.data()});
    skyCubemap.texture = tgai.createTexture(tga::TextureInfo(
        faceWidth, faceHeight, tga::Format::r8g8b8a8_srgb,
        tga::SamplerMode::linear, tga::AddressMode::clampEdge,
        tga::TextureType::_Cube, 6, staging, 0));
}