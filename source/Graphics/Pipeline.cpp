#include "Pipeline.hpp"
#include <spdlog/spdlog.h>
#include <regex>
#include "Compiler/Compiler.hpp"
#include "Context.hpp"
#include "Image.hpp"
#include "Scene/Object.hpp"
#include "Window/Window.hpp"

namespace {
vk::UniqueShaderModule CreateShaderModule(const std::vector<uint32_t>& code) {
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCode(code);
    return Context::getDevice().createShaderModuleUnique(createInfo);
}

vk::UniquePipeline CreateGraphicsPipeline(vk::ShaderModule vertModule,
                                          vk::ShaderModule fragModule,
                                          vk::PipelineLayout pipelineLayout,
                                          vk::RenderPass renderPass) {
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.setStage(vk::ShaderStageFlagBits::eVertex);
    vertShaderStageInfo.setModule(vertModule);
    vertShaderStageInfo.setPName("main");

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.setStage(vk::ShaderStageFlagBits::eFragment);
    fragShaderStageInfo.setModule(fragModule);
    fragShaderStageInfo.setPName("main");

    std::array shaderStages{vertShaderStageInfo, fragShaderStageInfo};

    vk::VertexInputBindingDescription bindingDescription;
    bindingDescription.setBinding(0);
    bindingDescription.setStride(sizeof(Vertex));
    bindingDescription.setInputRate(vk::VertexInputRate::eVertex);

    std::array attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
    vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList);

    auto width = static_cast<float>(Window::getWidth());
    auto height = static_cast<float>(Window::getHeight());
    vk::Viewport viewport{0.0f, 0.0f, width, height, 0.0f, 1.0f};
    vk::Rect2D scissor{{0, 0}, {Window::getWidth(), Window::getWidth()}};
    vk::PipelineViewportStateCreateInfo viewportState{{}, 1, &viewport, 1, &scissor};

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
    pipelineInfo.setRenderPass(renderPass);

    auto result = Context::getDevice().createGraphicsPipelineUnique({}, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a pipeline!");
    }
    return std::move(result.value);
}

vk::UniquePipeline CreateComputePipeline(vk::ShaderModule shaderModule,
                                         vk::ShaderStageFlagBits shaderStage,
                                         vk::PipelineLayout pipelineLayout) {
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

vk::UniquePipeline CreateRayTracingPipeline(
    const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages,
    const std::vector<vk::RayTracingShaderGroupCreateInfoKHR>& shaderGroups,
    vk::PipelineLayout pipelineLayout) {
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
    vk::DeviceAddress deviceAddress) {
    vk::StridedDeviceAddressRegionKHR region{};
    region.setDeviceAddress(deviceAddress);
    region.setStride(rtProperties.shaderGroupHandleAlignment);
    region.setSize(rtProperties.shaderGroupHandleAlignment);
    return region;
}
}  // namespace

void GraphicsPipeline::setup(Swapchain& swapchain, size_t pushSize) {
    assert(vertModule);
    assert(fragModule);
    this->pushSize = pushSize;
    pipelineLayout = descSet->createPipelineLayout(pushSize, vk::ShaderStageFlagBits::eAllGraphics);
    pipeline =
        CreateGraphicsPipeline(vertModule, fragModule, *pipelineLayout, swapchain.getRenderPass());
}

void GraphicsPipeline::bind(vk::CommandBuffer commandBuffer) {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    descSet->bind(commandBuffer, vk::PipelineBindPoint::eGraphics, *pipelineLayout);
}

void GraphicsPipeline::pushConstants(vk::CommandBuffer commandBuffer, void* pushData) {
    commandBuffer.pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, pushSize,
                                pushData);
}

void ComputePipeline::setup(size_t pushSize) {
    this->pushSize = pushSize;
    pipelineLayout = descSet->createPipelineLayout(pushSize, vk::ShaderStageFlagBits::eCompute);
    pipeline =
        CreateComputePipeline(shaderModule, vk::ShaderStageFlagBits::eCompute, *pipelineLayout);
}

void ComputePipeline::bind(vk::CommandBuffer commandBuffer) {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    descSet->bind(commandBuffer, vk::PipelineBindPoint::eCompute, *pipelineLayout);
}

void ComputePipeline::pushConstants(vk::CommandBuffer commandBuffer, void* pushData) {
    commandBuffer.pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, pushSize,
                                pushData);
}

void ComputePipeline::dispatch(vk::CommandBuffer commandBuffer,
                               uint32_t groupCountX,
                               uint32_t groupCountY) {
    commandBuffer.dispatch(groupCountX, groupCountY, 1);
}

void RayTracingPipeline::setup(size_t pushSize) {
    this->pushSize = pushSize;

    pipelineLayout = descSet->createPipelineLayout(
        pushSize, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
                      vk::ShaderStageFlagBits::eClosestHitKHR |
                      vk::ShaderStageFlagBits::eAnyHitKHR);
    pipeline = CreateRayTracingPipeline(shaderStages, shaderGroups, *pipelineLayout);

    // Get Ray Tracing Properties
    using vkRTP = vk::PhysicalDeviceRayTracingPipelinePropertiesKHR;
    auto rtProperties = Context::getPhysicalDevice()
                            .getProperties2<vk::PhysicalDeviceProperties2, vkRTP>()
                            .get<vkRTP>();

    // Calculate SBT size
    const uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    const size_t handleSizeAligned = rtProperties.shaderGroupHandleAlignment;
    const size_t groupCount = shaderGroups.size();
    const size_t sbtSize = groupCount * handleSizeAligned;

    // Get shader group handles
    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    const vk::Result result = Context::getDevice().getRayTracingShaderGroupHandlesKHR(
        *pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to get ray tracing shader group handles.");
    }

    // Create shader binding table
    raygenSBT = DeviceBuffer{BufferUsage::ShaderBindingTable, handleSize * rgenCount};
    missSBT = DeviceBuffer{BufferUsage::ShaderBindingTable, handleSize * missCount};
    hitSBT = DeviceBuffer{BufferUsage::ShaderBindingTable, handleSize * hitCount};

    raygenRegion = CreateAddressRegion(rtProperties, raygenSBT.getAddress());
    missRegion = CreateAddressRegion(rtProperties, missSBT.getAddress());
    hitRegion = CreateAddressRegion(rtProperties, hitSBT.getAddress());

    raygenSBT.copy(shaderHandleStorage.data() + 0 * handleSizeAligned);
    missSBT.copy(shaderHandleStorage.data() + 1 * handleSizeAligned);
    hitSBT.copy(shaderHandleStorage.data() + 2 * handleSizeAligned);
}

void RayTracingPipeline::bind(vk::CommandBuffer commandBuffer) {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *pipeline);
    descSet->bind(commandBuffer, vk::PipelineBindPoint::eRayTracingKHR, *pipelineLayout);
}

void RayTracingPipeline::pushConstants(vk::CommandBuffer commandBuffer, void* pushData) {
    commandBuffer.pushConstants(
        *pipelineLayout,
        vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
            vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
        0, pushSize, pushData);
}

void RayTracingPipeline::traceRays(vk::CommandBuffer commandBuffer,
                                   uint32_t countX,
                                   uint32_t countY) {
    commandBuffer.traceRaysKHR(raygenRegion, missRegion, hitRegion, {}, countX, countY, 1);
}
