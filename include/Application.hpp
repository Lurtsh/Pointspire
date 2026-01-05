#pragma once
#ifndef POINTSPIRE_APPLICATION_HPP
#define POINTSPIRE_APPLICATION_HPP

#include "tga/tga.hpp"
#include "tga/tga_math.hpp"

#include <vector>

#include "tga/tga_utils.hpp"

#include "PointCloud.hpp"
#include "Camera.hpp"
#include "Scene.hpp"

struct Application {
    tga::Interface tgai;
    tga::Window window;
    tga::Shader vertShader;
    tga::Shader fragShader;
    tga::Buffer uniformBuffer;
    tga::InputSet inputSet;
    tga::RenderPass renderPass;
    tga::CommandBuffer commandBuffer;

    //std::unique_ptr<PointCloud> bunnyModel;

    std::unique_ptr<Scene> scene;

    struct CameraData {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    Application() {
        auto [scrW, scrH] = tgai.screenResolution();
        auto winW = scrW / 2;
        auto winH = scrH / 2;

        scene = std::make_unique<Scene>(tgai);

        tga::WindowInfo winInfo{winW, winH, tga::PresentMode::vsync};
        window = tgai.createWindow(winInfo);
        tgai.setWindowTitle(window, "Pointspire");

        // Load the bunny model
        //std::vector<PositionVertex> bunnyVertices = loadBunny("assets/bunny/bun.conf", "assets/bunny/data/");

        //if (bunnyVertices.empty()) {
        //    std::cerr << "Failed to load bunny model.\n";
        //}

        //bunnyModel = std::make_unique<PointCloud>(tgai, bunnyVertices);

        // Load shaders
        // vertShader = tga::loadShader("shaders/bunny_primitive_vert.spv", tga::ShaderType::vertex, tgai);
        // fragShader = tga::loadShader("shaders/bunny_primitive_frag.spv", tga::ShaderType::fragment, tgai);
        // vertShader = tga::loadShader("shaders/cube_vert.spv", tga::ShaderType::vertex, tgai);
        // fragShader = tga::loadShader("shaders/cube_frag.spv", tga::ShaderType::fragment, tgai);
        vertShader = tga::loadShader("shaders/skybox_vert.spv", tga::ShaderType::vertex, tgai);
        fragShader = tga::loadShader("shaders/skybox_frag.spv", tga::ShaderType::fragment, tgai);

        glm::vec3 eye = {0.6f, 0.6f, 0.6f};
        glm::vec3 center = {0.5f, 0.5f, 0.5f};
        glm::vec3 up = {0.0f, 1.0f, 0.0f};

        cameraData.model = glm::mat4(1.0f);
        cameraData.view = glm::lookAt(eye, center, up);

        auto aspect = static_cast<float>(scrW) / static_cast<float>(scrH);

        cameraData.proj = glm::perspective_vk(glm::radians(60.0f), aspect, 0.1f, 100.0f);

        tga::BufferInfo uboInfo{
            tga::BufferUsage::uniform,
            sizeof(CameraData)
        };
        uniformBuffer = tgai.createBuffer(uboInfo);

        tga::InputLayout inputLayout{
            {
                tga::BindingLayout{tga::BindingType::uniformBuffer},
                // tga::BindingLayout{tga::BindingType::storageBuffer},
                tga::BindingLayout{tga::BindingType::sampler},
            }
        };

        tga::RenderPassInfo passInfo{
            vertShader,
            fragShader,
            window,
            {},
            inputLayout,
            tga::ClearOperation::all,
            tga::PerPixelOperations{tga::CompareOperation::less, true},
            tga::RasterizerConfig{tga::FrontFace::counterclockwise, tga::CullMode::none}
        };
        renderPass = tgai.createRenderPass(passInfo);

        tga::InputSetInfo inputSetInfo{
            renderPass,
            { tga::Binding{uniformBuffer, 0, 0},
                        // tga::Binding{bunnyModel->getBuffer(), 1, 0},
                        tga::Binding{scene.get()->getSkyCubemap(), 1, 0}
            },
            0
        };

        // inputSetInfo.targetPass = renderPass;
        inputSet = tgai.createInputSet(inputSetInfo);

        commandBuffer = tga::CommandBuffer{};
    }

    ~Application() {
        if (commandBuffer) tgai.free(commandBuffer);
        if (inputSet) tgai.free(inputSet);
        if (renderPass) tgai.free(renderPass);
        if (uniformBuffer) tgai.free(uniformBuffer);
        if (vertShader) tgai.free(vertShader);
        if (fragShader) tgai.free(fragShader);
        if (window) tgai.free(window);
    }

    void run();

private:
    CameraData cameraData;
};

#endif //POINTSPIRE_APPLICATION_HPP