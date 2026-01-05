#pragma once
#ifndef POINTSPIRE_SCENE_HPP
#define POINTSPIRE_SCENE_HPP

#include <string>

#include <tga/tga_utils.hpp>

#include "tga/tga.hpp"

/***
 * A class that manages all visible objects in the viewport, including:
 *  - Models
 *  - Textures, Skybox
 *  - Lighting
 * /
 */
class Scene {
public:
    Scene(tga::Interface& tgai);

    ~Scene() = default;

    // Skybox cubemap loader
    void loadSkyCubemap(const std::string filepath);
    void loadSkyCubemap(const std::vector<std::string> filepaths);
    const tga::Texture& getSkyCubemap() { return skyCubemap.texture; }
private:
    tga::Interface& tgai;

    // Skybox texture bundle
    tga::TextureBundle skyCubemap;
};

#endif //POINTSPIRE_SCENE_HPP