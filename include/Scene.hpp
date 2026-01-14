// #pragma once
// #ifndef POINTSPIRE_SCENE_HPP
// #define POINTSPIRE_SCENE_HPP
//
// #include <string>
//
// #include <tga/tga_utils.hpp>
//
// #include "tga/tga.hpp"
//
// /***
//  * A class that manages all visible objects in the viewport, including:
//  *  - Models
//  *  - Textures, Skybox
//  *  - Lighting
//  * /
//  */
// class Scene {
// public:
//     Scene(tga::Interface& tgai);
//
//     ~Scene() = default;
//
//     // Skybox cubemap loader
//     void loadSkyCubemap(const std::string filepath);
//     void loadSkyCubemap(const std::vector<std::string> filepaths);
//     const tga::Texture& getSkyCubemap() { return skyCubemap.texture; }
// private:
//     tga::Interface& tgai;
//
//     // Skybox texture bundle
//     tga::TextureBundle skyCubemap;
// };
//
// #endif //POINTSPIRE_SCENE_HPP

#pragma once
#ifndef POINTSPIRE_SCENE_HPP
#define POINTSPIRE_SCENE_HPP

#include <string>
#include <vector>
#include "tga/tga.hpp"
#include "tga/tga_utils.hpp"

class Scene {
public:
    Scene(tga::Interface& tgai);
    ~Scene();

    // Initialize the skybox rendering pipeline (Call this after creating the Camera UBO)
    void initSkyboxPipeline(tga::Window window, tga::Buffer cameraUBO);

    // Draw using Indirect Buffer
    void drawSkybox(tga::CommandRecorder& recorder, uint32_t frameIndex);

    const tga::Texture& getSkyCubemap() { return skyCubemap.texture; }

private:
    void loadSkyCubemap(const std::vector<std::string>& filepaths);

    tga::Interface& tgai;
    tga::TextureBundle skyCubemap{};

    // Skybox Rendering Resources
    tga::Shader m_vertShader;
    tga::Shader m_fragShader;
    tga::RenderPass m_renderPass;
    tga::InputSet m_inputSet;
    tga::Buffer m_indirectBuffer; // Indirect command for the cube
};

#endif //POINTSPIRE_SCENE_HPP