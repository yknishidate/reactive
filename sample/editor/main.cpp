#include <imgui_impl_vulkan.h>

#include <reactive/App.hpp>

using namespace rv;

std::string vertCode = R"(
#version 450
layout(location = 0) out vec4 outColor;
vec3 positions[] = vec3[](vec3(-1, 1, 0), vec3(0, -1, 0), vec3(1, 1, 0));
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
              .title = "HelloGraphics",
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
            if (ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags)) {
                ImGui::PopStyleVar(3);

                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu("File")) {
                        if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */
                        }
                        if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }

                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
                ImGui::End();
            }

            if (ImGui::Begin("Scrolling")) {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Important Stuff");
                for (int n = 0; n < 50; n++)
                    ImGui::Text("%04d: Some text", n);
                ImGui::End();
            }

            if (ImGui::Begin("Viewport")) {
                viewportResized = false;
                ImVec2 windowSize = ImGui::GetContentRegionAvail();
                if (viewportWidth != windowSize.x || viewportHeight != windowSize.y) {
                    viewportResized = true;
                    viewportWidth = windowSize.x;
                    viewportHeight = windowSize.y;
                    spdlog::info("size: {} {}", viewportWidth, viewportHeight);
                    createViewportImage(viewportWidth, viewportHeight);
                }
                ImGui::Image(imguiDescSet, windowSize, ImVec2(0, 1), ImVec2(1, 0));
                ImGui::End();
            }
        }
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        ShowFullscreenDockspace();

        commandBuffer.clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer.clearDepthStencilImage(getDefaultDepthImage(), 1.0f, 0);

        uint32_t renderWidth = static_cast<uint32_t>(viewportWidth);
        uint32_t renderHeight = static_cast<uint32_t>(viewportHeight);
        commandBuffer.setViewport(renderWidth, renderHeight);
        commandBuffer.setScissor(renderWidth, renderHeight);

        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);

        commandBuffer.beginRendering(imguiImage, getDefaultDepthImage(), {0, 0},
                                     {renderWidth, renderHeight});
        commandBuffer.draw(3, 1, 0, 0);
        commandBuffer.endRendering();
    }

    DescriptorSetHandle descSet;
    GraphicsPipelineHandle pipeline;

    ImageHandle viewportImage;

    bool viewportResized = false;
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
