#include <imgui_impl_vulkan.h>

#include <reactive/App.hpp>

using namespace rv;

std::string vertCode = R"(
#version 450
layout(location = 0) out vec4 outColor;
vec3 positions[] = vec3[](vec3(-1, -1, 0), vec3(0, 1, 0), vec3(1, -1, 0));
vec3 colors[] = vec3[](vec3(0), vec3(1, 0, 0), vec3(0, 1, 0));
void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 1);
    outColor = vec4(colors[gl_VertexIndex], 1);
})";

std::string fragCode = R"(
#version 450
layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = inColor;
})";

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 2560,
              .height = 1440,
              .title = "Reactive Editor",
              .layers = {Layer::Validation},
          }) {}

    void onStart() override {
        std::vector<ShaderHandle> shaders(2);
        shaders[0] = context.createShader({
            .code = Compiler::compileToSPV(vertCode, vk::ShaderStageFlagBits::eVertex),
            .stage = vk::ShaderStageFlagBits::eVertex,
        });

        shaders[1] = context.createShader({
            .code = Compiler::compileToSPV(fragCode, vk::ShaderStageFlagBits::eFragment),
            .stage = vk::ShaderStageFlagBits::eFragment,
        });

        descSet = context.createDescriptorSet({
            .shaders = shaders,
        });

        pipeline = context.createGraphicsPipeline({
            .descSetLayout = descSet->getLayout(),
            .vertexShader = shaders[0],
            .fragmentShader = shaders[1],
            .viewport = "dynamic",
            .scissor = "dynamic",
            .colorFormat = vk::Format::eR8G8B8A8Unorm,
        });

        viewportWidth = 1920;
        viewportHeight = 1080;
        createViewportImage(viewportWidth, viewportHeight);
    }

    void createViewportImage(uint32_t width, uint32_t height) {
        // TODO: use allocated memory
        imguiImage = context.createImage({
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage |
                     vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc |
                     vk::ImageUsageFlagBits::eColorAttachment,
            .extent = {width, height, 1},
            .format = vk::Format::eR8G8B8A8Unorm,
            .layout = vk::ImageLayout::eGeneral,
        });
        ImGui_ImplVulkan_RemoveTexture(imguiDescSet);
        imguiDescSet = ImGui_ImplVulkan_AddTexture(imguiImage->getSampler(), imguiImage->getView(),
                                                   VK_IMAGE_LAYOUT_GENERAL);
    }

    void ShowFullscreenDockspace() {
        static bool dockspaceOpen = true;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        window_flags |= ImGuiWindowFlags_MenuBar;

        if (dockspaceOpen) {
            if (ImGui::Begin("DockSpace", &dockspaceOpen, window_flags)) {
                ImGui::PopStyleVar(3);

                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu("File")) {
                        if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */
                        }
                        if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Edit")) {
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Create")) {
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }

                ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
                ImGui::End();
            }

            if (ImGui::Begin("Viewport")) {
                ImVec2 windowSize = ImGui::GetContentRegionAvail();
                viewportWidth = windowSize.x;
                viewportHeight = windowSize.y;
                vk::Extent3D extent = imguiImage->getExtent();
                ImGui::Image(imguiDescSet,
                             {static_cast<float>(extent.width), static_cast<float>(extent.height)},
                             ImVec2(0, 1), ImVec2(1, 0));
                ImGui::End();
            }

            {
                ImGui::Begin("Scene");
                if (ImGui::TreeNode("Some object 0")) {
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Some object 1")) {
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Some object 2")) {
                    ImGui::TreePop();
                }
                ImGui::End();
            }

            static float param0 = 0.0f;
            static float param1 = 0.0f;
            static float param2 = 0.0f;
            static float color[3] = {0.0f, 0.0f, 0.0f};
            {
                ImGui::Begin("Attribute");
                ImGui::SliderFloat("Some parameter 0", &param0, 0.0f, 1.0f);
                ImGui::SliderFloat("Some parameter 1", &param1, 0.0f, 1.0f);
                ImGui::SliderFloat("Some parameter 2", &param2, 0.0f, 1.0f);
                ImGui::ColorEdit3("Some color", color);
                ImGui::End();
            }

            if (ImGui::Begin("Project")) {
                for (int n = 0; n < 5; n++)
                    ImGui::Text("Asset");
                ImGui::End();
            }
        }
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        vk::Extent3D extent = imguiImage->getExtent();
        if (uint32_t(viewportWidth) != extent.width || uint32_t(viewportHeight) != extent.height) {
            context.getDevice().waitIdle();
            spdlog::info("Resized: {} {}", viewportWidth, viewportHeight);
            createViewportImage(viewportWidth, viewportHeight);
            extent = imguiImage->getExtent();
        }

        ShowFullscreenDockspace();

        commandBuffer.clearColorImage(imguiImage, {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer.transitionLayout(imguiImage, vk::ImageLayout::eGeneral);

        commandBuffer.clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.0f, 1.0f});
        commandBuffer.clearDepthStencilImage(getDefaultDepthImage(), 1.0f, 0);

        commandBuffer.setViewport(extent.width, extent.height);
        commandBuffer.setScissor(extent.width, extent.height);

        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);

        // TODO: create depth image
        commandBuffer.beginRendering(imguiImage, nullptr, {0, 0}, {extent.width, extent.height});
        commandBuffer.draw(3, 1, 0, 0);
        commandBuffer.endRendering();
    }

    DescriptorSetHandle descSet;
    GraphicsPipelineHandle pipeline;

    float viewportWidth;
    float viewportHeight;
    ImageHandle imguiImage;
    vk::DescriptorSet imguiDescSet;
    int testInt = 0;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
