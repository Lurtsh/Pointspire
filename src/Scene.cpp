#include "Scene.hpp"

Scene::Scene(tga::Interface &tgai) : tgai(tgai) {
    std::vector<std::string> faces = {
        "assets/textures/skybox/right.jpg",
        "assets/textures/skybox/left.jpg",
        "assets/textures/skybox/top.jpg",
        "assets/textures/skybox/bottom.jpg",
        "assets/textures/skybox/front.jpg",
        "assets/textures/skybox/back.jpg",
    };

    loadSkyCubemap(faces);
}

void Scene::loadSkyCubemap(const std::string filepath) {
    // skyCubemap = tga::loadTexture(filepath, tga::Format::r8g8b8a8_unorm, tga::SamplerMode::linear, tga::AddressMode::clampEdge, tgai);
    // stbi_load()

    auto loadTex = [&](tga::Texture& tex, std::string const& filepath, tga::Format format) {
        if (filepath.size() > 0) {
            int width, height, channels;
            stbi_uc *pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            if (pixels) {
                auto textureStagingBuffer = tgai.createStagingBuffer({width * height * 4, pixels});
                tex = tgai.createTexture(tga::TextureInfo(
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height), format,
                    tga::SamplerMode::nearest, tga::AddressMode::clampEdge,
                    tga::TextureType::_Cube, 6, textureStagingBuffer, 0));
                stbi_image_free(pixels);
            }
        }
    };
    loadTex(skyCubemap.texture, filepath, tga::Format::r8g8b8a8_srgb);
}

void Scene::loadSkyCubemap(std::vector<std::string> filepaths) {
    std::vector<uint8_t> cubeData;
    int width, height, channels;
    uint32_t faceWidth = 0;
    uint32_t faceHeight = 0;

    for (size_t i = 0; i < filepaths.size(); ++i) {
        stbi_uc *pixels = stbi_load(filepaths[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!pixels) {
            std::cerr << "Failed to load skybox face: " << filepaths[i] << std::endl;
            return;
        }

        if (i == 0) {
            faceWidth = static_cast<uint32_t>(width);
            faceHeight = static_cast<uint32_t>(height);

            cubeData.reserve(6 * faceWidth * faceHeight);
        } else {
            if (static_cast<uint32_t>(width) != faceWidth || static_cast<uint32_t>(height) != faceHeight) {
                std::cerr << "Error: Skybox face dimensions mismatch at " << filepaths[i] << std::endl;
                stbi_image_free(pixels);
                return;
            }
        }

        size_t faceSize = width * height * 4;
        cubeData.insert(cubeData.end(), pixels, pixels + faceSize);

        stbi_image_free(pixels);
    }

    auto stagingBuffer = tgai.createStagingBuffer({cubeData.size(), cubeData.data()});

    skyCubemap.texture = tgai.createTexture(tga::TextureInfo(
        faceWidth,
        faceHeight,
        tga::Format::r8g8b8a8_srgb,
        tga::SamplerMode::nearest,
        tga::AddressMode::clampEdge,
        tga::TextureType::_Cube,
        6,
        stagingBuffer,
        0));

    skyCubemap.width = faceWidth;
    skyCubemap.height = faceHeight;
}

