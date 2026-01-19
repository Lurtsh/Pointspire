#pragma once
#ifndef POINTSPIRE_SCENE_HPP
#define POINTSPIRE_SCENE_HPP

#include <string>
#include <vector>
#include "tga/tga.hpp"
#include "tga/tga_utils.hpp"

/**
 * @brief Manages environment resources, specifically the skybox.
 *
 * This class handles the loading of cubemap textures and the creation of
 * geometry buffers required to render the skybox background. It acts purely
 * as a resource container and does not manage render passes or pipelines.
 */
class Scene {
public:
    /**
     * @brief Constructs the Scene and initializes skybox resources.
     *
     * Loads the default set of skybox textures and creates the necessary
     * indirect draw buffers on the GPU.
     *
     * @param tgai Reference to the TGA interface for resource creation.
     */
    Scene(tga::Interface& tgai);

    ~Scene();

    /**
     * @brief Gets the loaded cubemap texture.
     * @return A const reference to the skybox texture handle.
     */
    const tga::Texture& getSkyCubemap() const { return skyCubemap.texture; }

    /**
     * @brief Gets the indirect draw buffer for the skybox.
     *
     * The buffer contains a single DrawIndirectCommand configured to draw
     * the 36 vertices of the skybox cube.
     *
     * @return A const reference to the indirect buffer.
     */
    const tga::Buffer& getIndirectBuffer() const { return m_indirectBuffer; }

private:
    /**
     * @brief Loads six individual images into a single Cubemap texture.
     *
     * @param filepaths A vector of 6 paths to the face images. Order must be:
     *                  Right, Left, Top, Bottom, Front, Back.
     */
    void loadSkyCubemap(const std::vector<std::string>& filepaths);

    tga::Interface& tgai;
    tga::TextureBundle skyCubemap{};
    tga::Buffer m_indirectBuffer;
};

#endif //POINTSPIRE_SCENE_HPP