#include "Pipeline.hpp"
#include <spdlog/spdlog.h>
#include <regex>
#include "Compiler/Compiler.hpp"
#include "Graphics/ArrayProxy.hpp"
#include "Image.hpp"
#include "RenderPass.hpp"
#include "Scene/Mesh.hpp"
#include "Scene/Object.hpp"

GraphicsPipeline::GraphicsPipeline(const Context* context, GraphicsPipelineCreateInfo createInfo)
    : Pipeline{context} {
    shaderStageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    bindPoint = vk::PipelineBindPoint::eGraphics;
    pushSize = createInfo.pushSize;

    vk::PushConstantRange pushRange;
    pushRange.setOffset(0);
    pushRange.setSize(pushSize);
    pushRange.setStageFlags(shaderStageFlags);

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.setSetLayouts(createInfo.descSetLayout);
    if (pushSize) {
        layoutInfo.setPushConstantRanges(pushRange);
    }
    pipelineLayout = context->getDevice().createPipelineLayoutUnique(layoutInfo);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages(2);
    shaderStages[0].setModule(createInfo.vertexShader->getModule());
    shaderStages[0].setStage(createInfo.vertexShader->getStage());
    shaderStages[0].setPName("main");
    shaderStages[1].setModule(createInfo.fragmentShader->getModule());
    shaderStages[1].setStage(createInfo.fragmentShader->getStage());
    shaderStages[1].setPName("main");

    vk::PipelineColorBlendAttachmentState colorBlendState;
    colorBlendState.setColorWriteMask(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.setAttachments(colorBlendState);
    colorBlending.setLogicOpEnable(VK_FALSE);
    vk::Viewport viewport{
        0.0f, 0.0f, static_cast<float>(createInfo.width), static_cast<float>(createInfo.height),
        0.0f, 1.0f};
    vk::Rect2D scissor{{0, 0}, {createInfo.width, createInfo.height}};
    vk::PipelineViewportStateCreateInfo viewportState{{}, 1, &viewport, 1, &scissor};

    vk::PipelineRasterizationStateCreateInfo rasterization;
    rasterization.setDepthClampEnable(VK_FALSE);
    rasterization.setRasterizerDiscardEnable(VK_FALSE);
    rasterization.setPolygonMode(createInfo.polygonMode);
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
    pipelineInfo.setRenderPass(createInfo.renderPass);

    vk::VertexInputBindingDescription bindingDescription;
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    if (type == Type::Graphics) {
        if (createInfo.vertexStride != 0) {
            bindingDescription.setBinding(0);
            bindingDescription.setStride(createInfo.vertexStride);
            bindingDescription.setInputRate(vk::VertexInputRate::eVertex);
            vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
            vertexInputInfo.setVertexAttributeDescriptions(createInfo.vertexAttributes);
        }
        inputAssembly.setTopology(createInfo.topology);
        pipelineInfo.setPInputAssemblyState(&inputAssembly);
        pipelineInfo.setPVertexInputState(&vertexInputInfo);
    }

    auto result = context->getDevice().createGraphicsPipelineUnique({}, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a pipeline!");
    }
    pipeline = std::move(result.value);
}

ComputePipeline::ComputePipeline(const Context* context, ComputePipelineCreateInfo createInfo) {
    shaderStageFlags = vk::ShaderStageFlagBits::eCompute;
    bindPoint = vk::PipelineBindPoint::eCompute;
    pushSize = createInfo.pushSize;

    vk::PushConstantRange pushRange;
    pushRange.setOffset(0);
    pushRange.setSize(pushSize);
    pushRange.setStageFlags(shaderStageFlags);

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.setSetLayouts(createInfo.descSetLayout);
    if (pushSize) {
        layoutInfo.setPushConstantRanges(pushRange);
    }
    pipelineLayout = context->getDevice().createPipelineLayoutUnique(layoutInfo);

    vk::PipelineShaderStageCreateInfo stage;
    stage.setStage(createInfo.computeShader->getStage());
    stage.setModule(createInfo.computeShader->getModule());
    stage.setPName("main");

    vk::ComputePipelineCreateInfo pipelineInfo;
    pipelineInfo.setStage(stage);
    pipelineInfo.setLayout(*pipelineLayout);
    auto res = context->getDevice().createComputePipelinesUnique({}, pipelineInfo);
    if (res.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a pipeline.");
    }
    pipeline = std::move(res.value.front());
}

void ComputePipeline::dispatch(vk::CommandBuffer commandBuffer,
                               uint32_t groupCountX,
                               uint32_t groupCountY,
                               uint32_t groupCountZ) const {
    commandBuffer.dispatch(groupCountX, groupCountY, groupCountZ);
}

RayTracingPipeline::RayTracingPipeline(const Context* context,
                                       RayTracingPipelineCreateInfo createInfo)
    : Pipeline{context} {
    shaderStageFlags =
        vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
        vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR |
        vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eCallableKHR;
    bindPoint = vk::PipelineBindPoint::eRayTracingKHR;
    pushSize = createInfo.pushSize;

    setShaders(createInfo.rgenShader, createInfo.missShader, createInfo.chitShader);

    vk::PushConstantRange pushRange;
    pushRange.setOffset(0);
    pushRange.setSize(pushSize);
    pushRange.setStageFlags(shaderStageFlags);

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.setSetLayouts(createInfo.descSetLayout);
    if (pushSize) {
        layoutInfo.setPushConstantRanges(pushRange);
    }
    pipelineLayout = context->getDevice().createPipelineLayoutUnique(layoutInfo);

    vk::RayTracingPipelineCreateInfoKHR pipelineInfo;
    pipelineInfo.setStages(shaderStages);
    pipelineInfo.setGroups(shaderGroups);
    pipelineInfo.setMaxPipelineRayRecursionDepth(createInfo.maxRayRecursionDepth);
    pipelineInfo.setLayout(*pipelineLayout);
    auto res =
        context->getDevice().createRayTracingPipelineKHRUnique(nullptr, nullptr, pipelineInfo);
    if (res.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a pipeline.");
    }
    pipeline = std::move(res.value);

    // Get Ray Tracing Properties
    using vkRTP = vk::PhysicalDeviceRayTracingPipelinePropertiesKHR;
    auto rtProperties = context->getPhysicalDevice()
                            .getProperties2<vk::PhysicalDeviceProperties2, vkRTP>()
                            .get<vkRTP>();

    // Calculate SBT size
    uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    size_t handleSizeAligned = rtProperties.shaderGroupHandleAlignment;
    size_t groupCount = shaderGroups.size();
    size_t sbtSize = groupCount * handleSizeAligned;

    // Get shader group handles
    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    vk::Result result = context->getDevice().getRayTracingShaderGroupHandlesKHR(
        *pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to get ray tracing shader group handles.");
    }

    // Create shader binding table
    raygenSBT = context->createDeviceBuffer({
        .usage = BufferUsage::ShaderBindingTable,
        .size = handleSize * rgenCount,
        .data = shaderHandleStorage.data() + 0 * handleSizeAligned,
    });
    missSBT = context->createDeviceBuffer({
        .usage = BufferUsage::ShaderBindingTable,
        .size = handleSize * missCount,
        .data = shaderHandleStorage.data() + 1 * handleSizeAligned,
    });
    hitSBT = context->createDeviceBuffer({
        .usage = BufferUsage::ShaderBindingTable,
        .size = handleSize * hitCount,
        .data = shaderHandleStorage.data() + 2 * handleSizeAligned,
    });

    raygenRegion.setDeviceAddress(raygenSBT.getAddress());
    raygenRegion.setStride(rtProperties.shaderGroupHandleAlignment);
    raygenRegion.setSize(rtProperties.shaderGroupHandleAlignment);

    missRegion.setDeviceAddress(missSBT.getAddress());
    missRegion.setStride(rtProperties.shaderGroupHandleAlignment);
    missRegion.setSize(rtProperties.shaderGroupHandleAlignment);

    hitRegion.setDeviceAddress(hitSBT.getAddress());
    hitRegion.setStride(rtProperties.shaderGroupHandleAlignment);
    hitRegion.setSize(rtProperties.shaderGroupHandleAlignment);
}

void RayTracingPipeline::traceRays(vk::CommandBuffer commandBuffer,
                                   uint32_t countX,
                                   uint32_t countY,
                                   uint32_t countZ) const {
    commandBuffer.traceRaysKHR(raygenRegion, missRegion, hitRegion, {}, countX, countY, countZ);
}
