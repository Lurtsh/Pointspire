#pragma once
#ifndef POINTSPIRE_APPLICATION_HPP
#define POINTSPIRE_APPLICATION_HPP

#include "tga/tga.hpp"
#include <memory>
#include <utility> // For std::pair

#include "PointCloud.hpp"
#include "Camera.hpp"
#include "Scene.hpp"

/**
 * @brief The main application class orchestrating the rendering engine.
 *
 * This class manages the application lifecycle, including window creation,
 * resource initialization (shaders, buffers, render passes), and the main
 * execution loop. It coordinates the interaction between the Camera,
 * Skybox (Scene), and the Point Cloud data.
 */
struct Application {
    /// @name Core Resources
    /// @{
    tga::Interface& tgai;           ///< Reference to the TGA Vulkan wrapper interface.
    tga::Window window;             ///< The OS window handle.
    tga::CommandBuffer commandBuffer; ///< Recyclable command buffer for frame commands.
    /// @}

    /// @name Point Cloud Render Pipeline
    /// @{
    tga::Shader pcVertShader;       ///< Vertex shader for point sprites (quads).
    tga::Shader pcFragShader;       ///< Fragment shader for point coloring.
    tga::RenderPass pcRenderPass;   ///< Pass config: Loads previous buffer, Writes Depth.
    tga::InputSet pcInputSet;       ///< Bindings: Camera UBO, Visible Point Storage Buffer.
    /// @}

    /// @name Skybox Render Pipeline
    /// @{
    tga::Shader skyVertShader;      ///< Vertex shader for the skybox cube.
    tga::Shader skyFragShader;      ///< Fragment shader for cubemap sampling.
    tga::RenderPass skyRenderPass;  ///< Pass config: Clears screen, Read-Only Depth.
    tga::InputSet skyInputSet;      ///< Bindings: Camera UBO, Cubemap Texture.
    /// @}

    /// @name Logical Components
    /// @{
    PointCloud pointCloud;          ///< Manages point data, normalization, and buffers.
    Camera camera;                  ///< Manages view/projection matrices and input.
    Scene scene;                    ///< Manages background assets (Skybox).
    /// @}

    /// @name Compute Culling Pipeline
    /// @{
    tga::Shader cullingShader;      ///< Compute shader for frustum culling and compaction.
    tga::Buffer cullingBuffer;      ///< (Unused variable/placeholder).
    tga::ComputePass cullPass;      ///< Pipeline state for the compute shader.
    tga::InputSet cullInputSet;     ///< Bindings: Cam, Source, Visible, Indirect, Info.
    /// @}

    /**
     * @brief Initializes the application, window, and all GPU resources.
     *
     * Sets up the rendering pipelines (Skybox first, PointCloud second) and
     * the compute pipeline for frustum culling.
     *
     * @param _tgai Reference to the initialized TGA interface.
     */
    Application(tga::Interface& _tgai);

    ~Application();

    /**
     * @brief Runs the main application loop.
     *
     * Handles input polling, delta time calculation, camera updates, compute dispatch,
     * and command buffer recording for every frame until the window is closed.
     */
    void run();

    /**
     * @brief Calculates 2D dispatch dimensions to bypass hardware limits.
     *
     * Most GPUs limit the X dimension of a dispatch group to 65535. This helper
     * converts a large 1D total count into a 2D (X, Y) grid layout.
     *
     * @param numThreads The total number of items (points) to process.
     * @param workGroupSize The local workgroup size defined in the shader (default 256).
     * @return std::pair<uint32_t, uint32_t> The {GroupCountX, GroupCountY} dimensions.
     */
    std::pair<uint32_t, uint32_t> getDispatchDimensions(size_t numThreads, uint32_t workGroupSize = 256);
};

#endif //POINTSPIRE_APPLICATION_HPP