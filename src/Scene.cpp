#include "Scene.hpp"
#include <iostream>

Scene::Scene(tga::Interface &tgai) : tgai(tgai) {
    // 1. Load Textures
    // Define the standard order for Cubemap faces (Right, Left, Top, Bottom, Front, Back)
    std::vector<std::string> faces = {
        "assets/textures/skybox/right.jpg", "assets/textures/skybox/left.jpg",
        "assets/textures/skybox/top.jpg",   "assets/textures/skybox/bottom.jpg",
        "assets/textures/skybox/front.jpg", "assets/textures/skybox/back.jpg",
    };
    loadSkyCubemap(faces);

    // 2. Create Indirect Buffer for Skybox
    // Defines a draw command for a cube (36 vertices) with 1 instance.
    tga::DrawIndirectCommand cmd{36, 1, 0, 0};
    tga::StagingBufferInfo cmdStaging{sizeof(cmd), reinterpret_cast<uint8_t*>(&cmd)};

    m_indirectBuffer = tgai.createBuffer({
        tga::BufferUsage::indirect | tga::BufferUsage::storage,
        sizeof(cmd),
        tgai.createStagingBuffer(cmdStaging)
    });
}

Scene::~Scene() {
    if (m_indirectBuffer) tgai.free(m_indirectBuffer);
    if (skyCubemap.texture) tgai.free(skyCubemap.texture);
}

void Scene::loadSkyCubemap(const std::vector<std::string>& filepaths) {
    std::vector<uint8_t> cubeData;
    int width, height, channels;
    uint32_t faceWidth = 0;
    uint32_t faceHeight = 0;

    // Iterate through all 6 faces and flatten their pixel data into a single buffer
    for (size_t i = 0; i < filepaths.size(); ++i) {
        stbi_uc *pixels = stbi_load(filepaths[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels) {
            std::cerr << "Failed to load texture: " << filepaths[i] << std::endl;
            continue;
        }

        // Initialize dimensions based on the first face
        if (i == 0) {
            faceWidth = width; faceHeight = height;
            // Pre-allocate memory for all 6 faces (Width * Height * 4 bytes per pixel * 6 faces)
            cubeData.reserve(6 * faceWidth * faceHeight * 4);
        }

        size_t faceSize = width * height * 4;
        cubeData.insert(cubeData.end(), pixels, pixels + faceSize);
        stbi_image_free(pixels);
    }

    if (cubeData.empty()) return;

    // Upload the combined data to the GPU as a Cubemap
    auto staging = tgai.createStagingBuffer({cubeData.size(), cubeData.data()});
    skyCubemap.texture = tgai.createTexture(tga::TextureInfo(
        faceWidth, faceHeight, tga::Format::r8g8b8a8_srgb,
        tga::SamplerMode::linear, tga::AddressMode::clampEdge,
        tga::TextureType::_Cube, 6, staging, 0));
}