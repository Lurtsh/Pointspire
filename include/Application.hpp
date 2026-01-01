#ifndef POINTSPIRE_APPLICATION_HPP
#define POINTSPIRE_APPLICATION_HPP

#include "tga/tga.hpp"
#include "tga/tga_math.hpp"

#include <vector>

#include "tga/tga_utils.hpp"

struct Application {
    tga::Interface tgai;
    tga::Window window;
    tga::Shader vertShader;
    tga::Shader fragShader;
    tga::Buffer uniformBuffer;
    tga::InputSet inputSet;
    tga::RenderPass renderPass;
    tga::CommandBuffer commandBuffer;

    struct CameraData {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    Application() {
        auto [scrW, scrH] = tgai.screenResolution();
        auto winW = scrW / 2;
        auto winH = scrH / 2;

        tga::WindowInfo winInfo{winW, winH, tga::PresentMode::vsync};
        window = tgai.createWindow(winInfo);
        tgai.setWindowTitle(window, "Pointspire");

        vertShader = tga::loadShader("shaders/cube_vert.spv", tga::ShaderType::vertex, tgai);
        fragShader = tga::loadShader("shaders/cube_frag.spv", tga::ShaderType::fragment, tgai);

        glm::vec3 eye = {2.5f, 2.5f, 2.5f};
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
            { tga::BindingLayout{tga::BindingType::uniformBuffer} }
        };

        tga::InputSetInfo inputSetInfo{
            tga::RenderPass{},
            { tga::Binding{uniformBuffer, 0, 0} },
            0
        };

        tga::RenderPassInfo passInfo{
            vertShader,
            fragShader,
            window,
            {},
            inputLayout,
            tga::ClearOperation::all,
            tga::PerPixelOperations{tga::CompareOperation::less, false},
            tga::RasterizerConfig{tga::FrontFace::counterclockwise, tga::CullMode::back}
        };
        renderPass = tgai.createRenderPass(passInfo);

        inputSetInfo.targetPass = renderPass;
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