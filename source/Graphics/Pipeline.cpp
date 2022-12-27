#include <regex>
#include <spdlog/spdlog.h>
#include "Context.hpp"
#include "Pipeline.hpp"
#include "Image.hpp"
#include "Window/Window.hpp"
#include "Compiler/Compiler.hpp"
#include "Scene/Object.hpp"

namespace
{
vk::UniqueShaderModule CreateShaderModule(const std::vector<uint32_t>& code)
{
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCode(code);
    return Context::getDevice().createShaderModuleUnique(createInfo);
}

vk::UniquePipeline CreateGraphicsPipeline(vk::ShaderModule vertModule, vk::ShaderModule fragModule,
                                          vk::PipelineLayout pipelineLayout)
{
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.setStage(vk::ShaderStageFlagBits::eVertex);
    vertShaderStageInfo.setModule(vertModule);
    vertShaderStageInfo.setPName("main");

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.setStage(vk::ShaderStageFlagBits::eFragment);
    fragShaderStageInfo.setModule(fragModule);
    fragShaderStageInfo.setPName("main");

    std::array shaderStages{ vertShaderStageInfo, fragShaderStageInfo };

    vk::VertexInputBindingDescription bindingDescription;
    bindingDescription.setBinding(0);
    bindingDescription.setStride(sizeof(Vertex));
    bindingDescription.setInputRate(vk::VertexInputRate::eVertex);

    std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions;
    attributeDescriptions[0].setBinding(0);
    attributeDescriptions[0].setLocation(0);
    attributeDescriptions[0].setFormat(vk::Format::eR32G32B32Sfloat);
    attributeDescriptions[0].setOffset(offsetof(Vertex, pos));

    attributeDescriptions[1].setBinding(0);
    attributeDescriptions[1].setLocation(1);
    attributeDescriptions[1].setFormat(vk::Format::eR32G32B32Sfloat);
    attributeDescriptions[1].setOffset(offsetof(Vertex, normal));

    attributeDescriptions[2].setBinding(0);
    attributeDescriptions[2].setLocation(2);
    attributeDescriptions[2].setFormat(vk::Format::eR32G32Sfloat);
    attributeDescriptions[2].setOffset(offsetof(Vertex, texCoord));

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
    vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList);

    auto width = static_cast<float>(Window::getWidth());
    auto height = static_cast<float>(Window::getHeight());
    vk::Viewport viewport{ 0.0f, 0.0f, width, height, 0.0f, 1.0f };
    vk::Rect2D scissor{ { 0, 0 }, {Window::getWidth(), Window::getWidth()} };
    vk::PipelineViewportStateCreateInfo viewportState{ {}, 1, &viewport, 1, &scissor };

    vk::PipelineRasterizationStateCreateInfo rasterization;
    rasterization.setDepthClampEnable(VK_FALSE);
    rasterization.setRasterizerDiscardEnable(VK_FALSE);
    rasterization.setPolygonMode(vk::PolygonMode::eFill);
    rasterization.setCullMode(vk::CullModeFlagBits::eNone);
    rasterization.setFrontFace(vk::FrontFace::eCounterClockwise);
    rasterization.setDepthBiasEnable(VK_FALSE);
    rasterization.setLineWidth(1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.setSampleShadingEnable(VK_FALSE);

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.setDepthTestEnable(VK_TRUE);
    depthStencil.setDepthWriteEnable(VK_TRUE);
    depthStencil.setDepthCompareOp(vk::CompareOp::eLess);
    depthStencil.setDepthBoundsTestEnable(VK_FALSE);
    depthStencil.setStencilTestEnable(VK_FALSE);

    using vkCC = vk::ColorComponentFlagBits;
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.setBlendEnable(VK_FALSE);
    colorBlendAttachment.setColorWriteMask(vkCC::eR | vkCC::eG | vkCC::eB | vkCC::eA);

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.setLogicOpEnable(VK_FALSE);
    colorBlending.setAttachments(colorBlendAttachment);

    vk::Format colorFormat = vk::Format::eB8G8R8A8Unorm;
    vk::Format depthFormat = vk::Format::eD32Sfloat;
    vk::PipelineRenderingCreateInfo renderingInfo;
    renderingInfo.setColorAttachmentCount(1);
    renderingInfo.setColorAttachmentFormats(colorFormat);
    renderingInfo.setDepthAttachmentFormat(depthFormat);

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.setStages(shaderStages);
    pipelineInfo.setPVertexInputState(&vertexInputInfo);
    pipelineInfo.setPInputAssemblyState(&inputAssembly);
    pipelineInfo.setPViewportState(&viewportState);
    pipelineInfo.setPRasterizationState(&rasterization);
    pipelineInfo.setPMultisampleState(&multisampling);
    pipelineInfo.setPDepthStencilState(&depthStencil);
    pipelineInfo.setPColorBlendState(&colorBlending);
    pipelineInfo.setLayout(pipelineLayout);
    pipelineInfo.setSubpass(0);
    pipelineInfo.setPNext(&renderingInfo);

    auto result = Context::getDevice().createGraphicsPipelineUnique({}, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a pipeline!");
    }
    return std::move(result.value);
}

vk::UniquePipeline CreateComputePipeline(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits shaderStage,
                                         vk::PipelineLayout pipelineLayout)
{
    vk::PipelineShaderStageCreateInfo stage;
    stage.setStage(shaderStage);
    stage.setModule(shaderModule);
    stage.setPName("main");

    vk::ComputePipelineCreateInfo createInfo;
    createInfo.setStage(stage);
    createInfo.setLayout(pipelineLayout);
    auto res = Context::getDevice().createComputePipelinesUnique({}, createInfo);
    if (res.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create ray tracing pipeline.");
    }
    return std::move(res.value.front());
}

vk::UniquePipeline CreateRayTracingPipeline(const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages,
                                            const std::vector<vk::RayTracingShaderGroupCreateInfoKHR>& shaderGroups,
                                            vk::PipelineLayout pipelineLayout)
{
    vk::RayTracingPipelineCreateInfoKHR createInfo;
    createInfo.setStages(shaderStages);
    createInfo.setGroups(shaderGroups);
    createInfo.setMaxPipelineRayRecursionDepth(4);
    createInfo.setLayout(pipelineLayout);
    auto res = Context::getDevice().createRayTracingPipelineKHRUnique(nullptr, nullptr, createInfo);
    if (res.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create ray tracing pipeline.");
    }
    return std::move(res.value);
}

vk::StridedDeviceAddressRegionKHR CreateAddressRegion(
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties,
    vk::DeviceAddress deviceAddress)
{
    vk::StridedDeviceAddressRegionKHR region{};
    region.setDeviceAddress(deviceAddress);
    region.setStride(rtProperties.shaderGroupHandleAlignment);
    region.setSize(rtProperties.shaderGroupHandleAlignment);
    return region;
}
} // namespace

void Pipeline::record(const std::string& name, const std::vector<Image>& images)
{
    registered = true;
    descSet->record(name, images);
}

void Pipeline::record(const std::string& name, const Buffer& buffer)
{
    registered = true;
    descSet->record(name, buffer);
}

void Pipeline::record(const std::string& name, const Image& image)
{
    registered = true;
    descSet->record(name, image);
}

void Pipeline::record(const std::string& name, const TopAccel& accel)
{
    registered = true;
    descSet->record(name, accel);
}

void GraphicsPipeline::loadShaders(const std::string& vertPath, const std::string& fragPath)
{
    spdlog::info("ComputePipeline::LoadShaders()");

    std::vector vertCode = Compiler::compileToSPV(vertPath);
    std::vector fragCode = Compiler::compileToSPV(fragPath);
    descSet->addBindingMap(vertCode, vk::ShaderStageFlagBits::eVertex);
    descSet->addBindingMap(fragCode, vk::ShaderStageFlagBits::eFragment);
    vertModule = CreateShaderModule(vertCode);
    fragModule = CreateShaderModule(fragCode);
}

void GraphicsPipeline::setup(size_t pushSize)
{
    this->pushSize = pushSize;
    if (registered) {
        pipelineLayout = descSet->createPipelineLayout(pushSize, vk::ShaderStageFlagBits::eAllGraphics);
    } else {
        pipelineLayout = Context::getDevice().createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{});
    }
    pipeline = CreateGraphicsPipeline(*vertModule, *fragModule, *pipelineLayout);
}

void GraphicsPipeline::begin(vk::CommandBuffer commandBuffer, void* pushData)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    if (registered) {
        descSet->bind(commandBuffer, vk::PipelineBindPoint::eGraphics, *pipelineLayout);
    }
    if (pushData) {
        commandBuffer.pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushSize, pushData);
    }

    vk::RenderingAttachmentInfo colorAttachment = Context::getSwapchain().getColorAttachmentInfo();
    //vk::RenderingAttachmentInfo depthStencilAttachment = Context::GetSwapchain().GetDepthAttachmentInfo();

    vk::RenderingInfo renderingInfo;
    renderingInfo.setRenderArea({ {0, 0}, {Window::getWidth(), Window::getHeight()} });
    renderingInfo.setLayerCount(1);
    renderingInfo.setColorAttachments(colorAttachment);
    //renderingInfo.setPDepthAttachment(&depthStencilAttachment);

    commandBuffer.beginRendering(renderingInfo);
}

void GraphicsPipeline::end(vk::CommandBuffer commandBuffer)
{
    commandBuffer.endRendering();
}

void ComputePipeline::loadShaders(const std::string& path)
{
    spdlog::info("ComputePipeline::LoadShaders()");

    std::vector spirvCode = Compiler::compileToSPV(path);
    descSet->addBindingMap(spirvCode, vk::ShaderStageFlagBits::eCompute);
    shaderModule = CreateShaderModule(spirvCode);
}

void ComputePipeline::setup(size_t pushSize)
{
    this->pushSize = pushSize;
    pipelineLayout = descSet->createPipelineLayout(pushSize, vk::ShaderStageFlagBits::eCompute);
    pipeline = CreateComputePipeline(*shaderModule, vk::ShaderStageFlagBits::eCompute, *pipelineLayout);
}

void ComputePipeline::run(vk::CommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, void* pushData)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    descSet->bind(commandBuffer, vk::PipelineBindPoint::eCompute, *pipelineLayout);
    if (pushData) {
        commandBuffer.pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, pushSize, pushData);
    }
    commandBuffer.dispatch(groupCountX, groupCountY, 1);
}

void RayTracingPipeline::loadShaders(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath)
{
    spdlog::info("RayTracingPipeline::LoadShaders()");
    rgenCount = 1;
    missCount = 1;
    hitCount = 1;

    const uint32_t rgenIndex = 0;
    const uint32_t missIndex = 1;
    const uint32_t chitIndex = 2;

    // Compile shaders
    std::vector rgenShader = Compiler::compileToSPV(rgenPath);
    std::vector missShader = Compiler::compileToSPV(missPath);
    std::vector chitShader = Compiler::compileToSPV(chitPath);

    // Get bindings
    descSet->addBindingMap(rgenShader, vk::ShaderStageFlagBits::eRaygenKHR);
    descSet->addBindingMap(missShader, vk::ShaderStageFlagBits::eMissKHR);
    descSet->addBindingMap(chitShader, vk::ShaderStageFlagBits::eClosestHitKHR);

    shaderModules.push_back(CreateShaderModule(rgenShader));
    shaderStages.push_back({ {}, vk::ShaderStageFlagBits::eRaygenKHR, *shaderModules.back(), "main" });
    shaderGroups.push_back({ vk::RayTracingShaderGroupTypeKHR::eGeneral,
                           rgenIndex, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR });

    shaderModules.push_back(CreateShaderModule(missShader));
    shaderStages.push_back({ {}, vk::ShaderStageFlagBits::eMissKHR, *shaderModules.back(), "main" });
    shaderGroups.push_back({ vk::RayTracingShaderGroupTypeKHR::eGeneral,
                           missIndex, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR });

    shaderModules.push_back(CreateShaderModule(chitShader));
    shaderStages.push_back({ {}, vk::ShaderStageFlagBits::eClosestHitKHR, *shaderModules.back(), "main" });
    shaderGroups.push_back({ vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                           VK_SHADER_UNUSED_KHR, chitIndex, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR });

}

void RayTracingPipeline::loadShaders(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath, const std::string& ahitPath)
{
    spdlog::info("RayTracingPipeline::LoadShaders()");
    rgenCount = 1;
    missCount = 1;
    hitCount = 2;

    const uint32_t rgenIndex = 0;
    const uint32_t missIndex = 1;
    const uint32_t chitIndex = 2;
    const uint32_t ahitIndex = 3;

    // Compile shaders
    std::vector rgenShader = Compiler::compileToSPV(rgenPath);
    std::vector missShader = Compiler::compileToSPV(missPath);
    std::vector chitShader = Compiler::compileToSPV(chitPath);
    std::vector ahitShader = Compiler::compileToSPV(ahitPath);

    // Get bindings
    descSet->addBindingMap(rgenShader, vk::ShaderStageFlagBits::eRaygenKHR);
    descSet->addBindingMap(missShader, vk::ShaderStageFlagBits::eMissKHR);
    descSet->addBindingMap(chitShader, vk::ShaderStageFlagBits::eClosestHitKHR);
    descSet->addBindingMap(ahitShader, vk::ShaderStageFlagBits::eAnyHitKHR);


    shaderModules.push_back(CreateShaderModule(rgenShader));
    shaderStages.push_back({ {}, vk::ShaderStageFlagBits::eRaygenKHR, *shaderModules.back(), "main" });
    shaderGroups.push_back({ vk::RayTracingShaderGroupTypeKHR::eGeneral,
                           rgenIndex, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR });

    shaderModules.push_back(CreateShaderModule(missShader));
    shaderStages.push_back({ {}, vk::ShaderStageFlagBits::eMissKHR, *shaderModules.back(), "main" });
    shaderGroups.push_back({ vk::RayTracingShaderGroupTypeKHR::eGeneral,
                           missIndex, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR });

    shaderModules.push_back(CreateShaderModule(chitShader));
    shaderStages.push_back({ {}, vk::ShaderStageFlagBits::eClosestHitKHR, *shaderModules.back(), "main" });
    shaderModules.push_back(CreateShaderModule(ahitShader));
    shaderStages.push_back({ {}, vk::ShaderStageFlagBits::eAnyHitKHR, *shaderModules.back(), "main" });
    shaderGroups.push_back({ vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                           VK_SHADER_UNUSED_KHR, chitIndex, ahitIndex, VK_SHADER_UNUSED_KHR });
}

void RayTracingPipeline::setup(size_t pushSize)
{
    this->pushSize = pushSize;

    pipelineLayout = descSet->createPipelineLayout(pushSize,
                                                   vk::ShaderStageFlagBits::eRaygenKHR |
                                                   vk::ShaderStageFlagBits::eMissKHR |
                                                   vk::ShaderStageFlagBits::eClosestHitKHR |
                                                   vk::ShaderStageFlagBits::eAnyHitKHR);
    pipeline = CreateRayTracingPipeline(shaderStages, shaderGroups, *pipelineLayout);

    // Get Ray Tracing Properties
    using vkRTP = vk::PhysicalDeviceRayTracingPipelinePropertiesKHR;
    auto rtProperties = Context::getPhysicalDevice().getProperties2<vk::PhysicalDeviceProperties2, vkRTP>().get<vkRTP>();

    // Calculate SBT size
    const uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    const size_t handleSizeAligned = rtProperties.shaderGroupHandleAlignment;
    const size_t groupCount = shaderGroups.size();
    const size_t sbtSize = groupCount * handleSizeAligned;

    // Get shader group handles
    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    const vk::Result result = Context::getDevice().getRayTracingShaderGroupHandlesKHR(*pipeline, 0, groupCount, sbtSize,
                                                                                      shaderHandleStorage.data());
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to get ray tracing shader group handles.");
    }

    // Create shader binding table
    vk::BufferUsageFlags usage =
        vk::BufferUsageFlagBits::eShaderBindingTableKHR |
        vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eShaderDeviceAddress;

    raygenSBT = DeviceBuffer{ usage, handleSize * rgenCount };
    missSBT = DeviceBuffer{ usage, handleSize * missCount };
    hitSBT = DeviceBuffer{ usage, handleSize * hitCount };

    raygenRegion = CreateAddressRegion(rtProperties, raygenSBT.getAddress());
    missRegion = CreateAddressRegion(rtProperties, missSBT.getAddress());
    hitRegion = CreateAddressRegion(rtProperties, hitSBT.getAddress());

    raygenSBT.copy(shaderHandleStorage.data() + 0 * handleSizeAligned);
    missSBT.copy(shaderHandleStorage.data() + 1 * handleSizeAligned);
    hitSBT.copy(shaderHandleStorage.data() + 2 * handleSizeAligned);
}

void RayTracingPipeline::run(vk::CommandBuffer commandBuffer, uint32_t countX, uint32_t countY, void* pushData)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *pipeline);
    descSet->bind(commandBuffer, vk::PipelineBindPoint::eRayTracingKHR, *pipelineLayout);
    if (pushData) {
        commandBuffer.pushConstants(*pipelineLayout,
                                    vk::ShaderStageFlagBits::eRaygenKHR |
                                    vk::ShaderStageFlagBits::eMissKHR |
                                    vk::ShaderStageFlagBits::eClosestHitKHR |
                                    vk::ShaderStageFlagBits::eAnyHitKHR,
                                    0, pushSize, pushData);
    }
    commandBuffer.traceRaysKHR(raygenRegion, missRegion, hitRegion, {}, countX, countY, 1);
}
