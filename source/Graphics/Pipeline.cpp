#include "Pipeline.hpp"
#include <spdlog/spdlog.h>
#include <regex>
#include "Compiler/Compiler.hpp"
#include "Context.hpp"
#include "Graphics/ArrayProxy.hpp"
#include "Image.hpp"
#include "RenderPass.hpp"
#include "Scene/Mesh.hpp"
#include "Scene/Object.hpp"
#include "Window/Window.hpp"

namespace {
// vk::UniquePipeline createComputePipeline(vk::ShaderModule shaderModule,
//                                          vk::ShaderStageFlagBits shaderStage,
//                                          vk::PipelineLayout pipelineLayout) {
//     vk::PipelineShaderStageCreateInfo stage;
//     stage.setStage(shaderStage);
//     stage.setModule(shaderModule);
//     stage.setPName("main");
//
//     vk::ComputePipelineCreateInfo createInfo;
//     createInfo.setStage(stage);
//     createInfo.setLayout(pipelineLayout);
//     auto res = Context::getDevice().createComputePipelinesUnique({}, createInfo);
//     if (res.result != vk::Result::eSuccess) {
//         throw std::runtime_error("failed to create ray tracing pipeline.");
//     }
//     return std::move(res.value.front());
// }
//
// vk::UniquePipeline createRayTracingPipeline(
//     const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages,
//     const std::vector<vk::RayTracingShaderGroupCreateInfoKHR>& shaderGroups,
//     vk::PipelineLayout pipelineLayout) {
//     vk::RayTracingPipelineCreateInfoKHR createInfo;
//     createInfo.setStages(shaderStages);
//     createInfo.setGroups(shaderGroups);
//     createInfo.setMaxPipelineRayRecursionDepth(4);
//     createInfo.setLayout(pipelineLayout);
//     auto res = Context::getDevice().createRayTracingPipelineKHRUnique(nullptr, nullptr,
//     createInfo); if (res.result != vk::Result::eSuccess) {
//         throw std::runtime_error("failed to create ray tracing pipeline.");
//     }
//     return std::move(res.value);
// }

vk::StridedDeviceAddressRegionKHR createAddressRegion(
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties,
    vk::DeviceAddress deviceAddress) {
    vk::StridedDeviceAddressRegionKHR region{};
    region.setDeviceAddress(deviceAddress);
    region.setStride(rtProperties.shaderGroupHandleAlignment);
    region.setSize(rtProperties.shaderGroupHandleAlignment);
    return region;
}
}  // namespace

// void GraphicsPipeline::setup(RenderPass& renderPass) {
//     this->pushSize = pushSize;
//     pipelineLayout = descSet->createPipelineLayout(pushSize,
//     vk::ShaderStageFlagBits::eAllGraphics |
//                                                                  vk::ShaderStageFlagBits::eMeshEXT
//                                                                  |
//                                                                  vk::ShaderStageFlagBits::eTaskEXT);
//
//     std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
//     bool useMeshShader = false;
//     for (auto& shader : shaders) {
//         useMeshShader |= shader->getStage() == vk::ShaderStageFlagBits::eMeshEXT;
//         shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
//                                    .setModule(shader->getModule())
//                                    .setStage(shader->getStage())
//                                    .setPName("main"));
//     }
//     pipeline = createGraphicsPipeline(shaderStages, *pipelineLayout, renderPass.getRenderPass(),
//                                       renderPass.createColorBlending(), topology, polygonMode,
//                                       useMeshShader);
// }

void GraphicsPipeline::setup(vk::RenderPass renderPass) {
    this->pushSize = pushSize;
    pipelineLayout = descSet->createPipelineLayout(pushSize, vk::ShaderStageFlagBits::eAllGraphics |
                                                                 vk::ShaderStageFlagBits::eMeshEXT |
                                                                 vk::ShaderStageFlagBits::eTaskEXT);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    bool useMeshShader = false;
    for (auto& shader : shaders) {
        useMeshShader |= shader->getStage() == vk::ShaderStageFlagBits::eMeshEXT;
        shaderStages.push_back(vk::PipelineShaderStageCreateInfo()
                                   .setModule(shader->getModule())
                                   .setStage(shader->getStage())
                                   .setPName("main"));
    }
    vk::PipelineColorBlendAttachmentState colorBlendState;
    colorBlendState.setColorWriteMask(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.setAttachments(colorBlendState);
    colorBlending.setLogicOpEnable(VK_FALSE);
    // pipeline = createGraphicsPipeline(shaderStages, *pipelineLayout, renderPass, colorBlending,
    //                                   topology, polygonMode, useMeshShader);
    auto width = static_cast<float>(m_app->getWidth());
    auto height = static_cast<float>(m_app->getHeight());
    vk::Viewport viewport{0.0f, 0.0f, width, height, 0.0f, 1.0f};
    vk::Rect2D scissor{{0, 0}, {m_app->getWidth(), m_app->getWidth()}};
    vk::PipelineViewportStateCreateInfo viewportState{{}, 1, &viewport, 1, &scissor};

    vk::PipelineRasterizationStateCreateInfo rasterization;
    rasterization.setDepthClampEnable(VK_FALSE);
    rasterization.setRasterizerDiscardEnable(VK_FALSE);
    rasterization.setPolygonMode(polygonMode);
    rasterization.setCullMode(vk::CullModeFlagBits::eNone);
    rasterization.setFrontFace(vk::FrontFace::eCounterClockwise);
    rasterization.setDepthBiasEnable(VK_FALSE);
    rasterization.setLineWidth(2.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.setSampleShadingEnable(VK_FALSE);

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.setDepthTestEnable(VK_TRUE);
    depthStencil.setDepthWriteEnable(VK_TRUE);
    depthStencil.setDepthCompareOp(vk::CompareOp::eLess);
    depthStencil.setDepthBoundsTestEnable(VK_FALSE);
    depthStencil.setStencilTestEnable(VK_FALSE);

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.setStages(shaderStages);
    pipelineInfo.setPViewportState(&viewportState);
    pipelineInfo.setPRasterizationState(&rasterization);
    pipelineInfo.setPMultisampleState(&multisampling);
    pipelineInfo.setPDepthStencilState(&depthStencil);
    pipelineInfo.setPColorBlendState(&colorBlending);
    pipelineInfo.setLayout(*pipelineLayout);
    pipelineInfo.setSubpass(0);
    pipelineInfo.setRenderPass(renderPass);

    vk::VertexInputBindingDescription bindingDescription;
    std::array attributeDescriptions = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    if (!useMeshShader) {
        bindingDescription.setBinding(0);
        bindingDescription.setStride(sizeof(Vertex));
        bindingDescription.setInputRate(vk::VertexInputRate::eVertex);
        vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
        vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);
        inputAssembly.setTopology(topology);
        pipelineInfo.setPInputAssemblyState(&inputAssembly);
        pipelineInfo.setPVertexInputState(&vertexInputInfo);
    }

    auto result = m_app->getDevice().createGraphicsPipelineUnique({}, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a pipeline!");
    }
    pipeline = std::move(result.value);
}

void GraphicsPipeline::bind(vk::CommandBuffer commandBuffer) {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    descSet->bind(commandBuffer, vk::PipelineBindPoint::eGraphics, *pipelineLayout);
}

void GraphicsPipeline::pushConstants(vk::CommandBuffer commandBuffer, const void* pushData) {
    commandBuffer.pushConstants(*pipelineLayout,
                                vk::ShaderStageFlagBits::eAllGraphics |
                                    vk::ShaderStageFlagBits::eMeshEXT |
                                    vk::ShaderStageFlagBits::eTaskEXT,
                                0, pushSize, pushData);
}

// void ComputePipeline::setup() {
//     pipelineLayout = descSet->createPipelineLayout(pushSize, vk::ShaderStageFlagBits::eCompute);
//     pipeline =
//         createComputePipeline(shaderModule, vk::ShaderStageFlagBits::eCompute, *pipelineLayout);
// }
//
// void ComputePipeline::bind(vk::CommandBuffer commandBuffer) {
//     commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
//     descSet->bind(commandBuffer, vk::PipelineBindPoint::eCompute, *pipelineLayout);
// }
//
// void ComputePipeline::pushConstants(vk::CommandBuffer commandBuffer, const void* pushData) {
//     commandBuffer.pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, pushSize,
//                                 pushData);
// }
//
// void ComputePipeline::dispatch(vk::CommandBuffer commandBuffer,
//                                uint32_t groupCountX,
//                                uint32_t groupCountY) {
//     commandBuffer.dispatch(groupCountX, groupCountY, 1);
// }
//
// void RayTracingPipeline::setup() {
//     pipelineLayout = descSet->createPipelineLayout(
//         pushSize, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
//                       vk::ShaderStageFlagBits::eClosestHitKHR |
//                       vk::ShaderStageFlagBits::eAnyHitKHR);
//     pipeline = createRayTracingPipeline(shaderStages, shaderGroups, *pipelineLayout);
//
//     // Get Ray Tracing Properties
//     using vkRTP = vk::PhysicalDeviceRayTracingPipelinePropertiesKHR;
//     auto rtProperties = Context::getPhysicalDevice()
//                             .getProperties2<vk::PhysicalDeviceProperties2, vkRTP>()
//                             .get<vkRTP>();
//
//     // Calculate SBT size
//     uint32_t handleSize = rtProperties.shaderGroupHandleSize;
//     size_t handleSizeAligned = rtProperties.shaderGroupHandleAlignment;
//     size_t groupCount = shaderGroups.size();
//     size_t sbtSize = groupCount * handleSizeAligned;
//
//     // Get shader group handles
//     std::vector<uint8_t> shaderHandleStorage(sbtSize);
//     vk::Result result = Context::getDevice().getRayTracingShaderGroupHandlesKHR(
//         *pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());
//     if (result != vk::Result::eSuccess) {
//         throw std::runtime_error("failed to get ray tracing shader group handles.");
//     }
//
//     // Create shader binding table
//     raygenSBT = DeviceBuffer{BufferUsage::ShaderBindingTable, handleSize * rgenCount};
//     missSBT = DeviceBuffer{BufferUsage::ShaderBindingTable, handleSize * missCount};
//     hitSBT = DeviceBuffer{BufferUsage::ShaderBindingTable, handleSize * hitCount};
//
//     raygenRegion = createAddressRegion(rtProperties, raygenSBT.getAddress());
//     missRegion = createAddressRegion(rtProperties, missSBT.getAddress());
//     hitRegion = createAddressRegion(rtProperties, hitSBT.getAddress());
//
//     raygenSBT.copy(shaderHandleStorage.data() + 0 * handleSizeAligned);
//     missSBT.copy(shaderHandleStorage.data() + 1 * handleSizeAligned);
//     hitSBT.copy(shaderHandleStorage.data() + 2 * handleSizeAligned);
// }
//
// void RayTracingPipeline::bind(vk::CommandBuffer commandBuffer) {
//     commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *pipeline);
//     descSet->bind(commandBuffer, vk::PipelineBindPoint::eRayTracingKHR, *pipelineLayout);
// }
//
// void RayTracingPipeline::pushConstants(vk::CommandBuffer commandBuffer, const void* pushData) {
//     commandBuffer.pushConstants(
//         *pipelineLayout,
//         vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
//             vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
//         0, pushSize, pushData);
// }
//
// void RayTracingPipeline::traceRays(vk::CommandBuffer commandBuffer,
//                                    uint32_t countX,
//                                    uint32_t countY) {
//     commandBuffer.traceRaysKHR(raygenRegion, missRegion, hitRegion, {}, countX, countY, 1);
// }
