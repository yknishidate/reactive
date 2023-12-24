#include "Graphics/Pipeline.hpp"

#include <spdlog/spdlog.h>
#include <regex>

#include "Compiler/Compiler.hpp"
#include "Graphics/ArrayProxy.hpp"
#include "Graphics/CommandBuffer.hpp"
#include "Graphics/Image.hpp"
#include "Scene/Mesh.hpp"
#include "Scene/Object.hpp"

namespace rv {
GraphicsPipeline::GraphicsPipeline(const Context* context, GraphicsPipelineCreateInfo createInfo)
    : Pipeline{context} {
    shaderStageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    bindPoint = vk::PipelineBindPoint::eGraphics;
    pushSize = createInfo.pushSize;

    vk::PushConstantRange pushRange;
    pushRange.setOffset(0);
    pushRange.setSize(createInfo.pushSize);
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

    // Pipeline states
    std::vector<vk::DynamicState> dynamicStates;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.setViewportCount(1);
    dynamicStates.push_back(vk::DynamicState::eViewport);
    viewportState.setScissorCount(1);
    dynamicStates.push_back(vk::DynamicState::eScissor);

    vk::PipelineRasterizationStateCreateInfo rasterization;
    rasterization.setDepthClampEnable(VK_FALSE);
    rasterization.setRasterizerDiscardEnable(VK_FALSE);
    rasterization.setDepthBiasEnable(VK_FALSE);

    if (std::holds_alternative<vk::PolygonMode>(createInfo.polygonMode)) {
        rasterization.setPolygonMode(std::get<vk::PolygonMode>(createInfo.polygonMode));
    } else {
        assert(std::get<std::string>(createInfo.polygonMode) == "dynamic");
        dynamicStates.push_back(vk::DynamicState::ePolygonModeEXT);
    }

    if (std::holds_alternative<vk::FrontFace>(createInfo.frontFace)) {
        rasterization.setFrontFace(std::get<vk::FrontFace>(createInfo.frontFace));
    } else {
        assert(std::get<std::string>(createInfo.frontFace) == "dynamic");
        dynamicStates.push_back(vk::DynamicState::eFrontFace);
    }

    if (std::holds_alternative<vk::CullModeFlags>(createInfo.cullMode)) {
        rasterization.setCullMode(std::get<vk::CullModeFlags>(createInfo.cullMode));
    } else {
        assert(std::get<std::string>(createInfo.cullMode) == "dynamic");
        dynamicStates.push_back(vk::DynamicState::eCullMode);
    }

    if (std::holds_alternative<float>(createInfo.lineWidth)) {
        rasterization.setLineWidth(std::get<float>(createInfo.lineWidth));
    } else {
        assert(std::get<std::string>(createInfo.lineWidth) == "dynamic");
        dynamicStates.push_back(vk::DynamicState::eLineWidth);
    }

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.setSampleShadingEnable(VK_FALSE);

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.setDepthTestEnable(VK_TRUE);
    depthStencil.setDepthWriteEnable(VK_TRUE);
    depthStencil.setDepthCompareOp(vk::CompareOp::eLess);
    depthStencil.setDepthBoundsTestEnable(VK_FALSE);
    depthStencil.setStencilTestEnable(VK_FALSE);

    vk::PipelineRenderingCreateInfo renderingInfo;
    renderingInfo.setColorAttachmentFormats(createInfo.colorFormats);
    renderingInfo.setDepthAttachmentFormat(createInfo.depthFormat);

    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendStates;
    for (uint32_t i = 0; i < createInfo.colorFormats.size(); i++) {
        vk::PipelineColorBlendAttachmentState colorBlendState;
        colorBlendState.setColorWriteMask(
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
        if (createInfo.alphaBlending) {
            colorBlendState.setBlendEnable(true);
            colorBlendState.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha);
            colorBlendState.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
            colorBlendState.setColorBlendOp(vk::BlendOp::eAdd);
            colorBlendState.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
            colorBlendState.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
            colorBlendState.setAlphaBlendOp(vk::BlendOp::eAdd);
        }
        colorBlendStates.push_back(colorBlendState);
    }

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.setAttachments(colorBlendStates);
    colorBlending.setLogicOpEnable(VK_FALSE);

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.setStages(shaderStages);
    pipelineInfo.setPViewportState(&viewportState);
    pipelineInfo.setPRasterizationState(&rasterization);
    pipelineInfo.setPMultisampleState(&multisampling);
    pipelineInfo.setPDepthStencilState(&depthStencil);
    pipelineInfo.setPColorBlendState(&colorBlending);
    pipelineInfo.setLayout(*pipelineLayout);
    pipelineInfo.setSubpass(0);
    pipelineInfo.setPNext(&renderingInfo);

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
    vk::VertexInputBindingDescription bindingDescription;
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    std::vector<vk::VertexInputAttributeDescription> attributes;

    if (createInfo.vertexStride != 0) {
        bindingDescription.setBinding(0);
        bindingDescription.setStride(createInfo.vertexStride);
        bindingDescription.setInputRate(vk::VertexInputRate::eVertex);

        attributes.resize(createInfo.vertexAttributes.size());
        int i = 0;
        for (auto& attribute : createInfo.vertexAttributes) {
            attributes[i].setBinding(0);
            attributes[i].setLocation(i);
            attributes[i].setFormat(attribute.format);
            attributes[i].setOffset(attribute.offset);
            i++;
        }

        vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
        vertexInputInfo.setVertexAttributeDescriptions(attributes);
    }
    dynamicStateInfo.setDynamicStates(dynamicStates);
    inputAssembly.setTopology(createInfo.topology);
    pipelineInfo.setPInputAssemblyState(&inputAssembly);
    pipelineInfo.setPVertexInputState(&vertexInputInfo);
    pipelineInfo.setPDynamicState(&dynamicStateInfo);

    auto result = context->getDevice().createGraphicsPipelineUnique({}, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a pipeline!");
    }
    pipeline = std::move(result.value);
}

MeshShaderPipeline::MeshShaderPipeline(const Context* context,
                                       MeshShaderPipelineCreateInfo createInfo)
    : Pipeline{context} {
    shaderStageFlags = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT |
                       vk::ShaderStageFlagBits::eFragment;
    bindPoint = vk::PipelineBindPoint::eGraphics;
    pushSize = createInfo.pushSize;

    vk::PushConstantRange pushRange;
    pushRange.setOffset(0);
    pushRange.setSize(createInfo.pushSize);
    pushRange.setStageFlags(shaderStageFlags);

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.setSetLayouts(createInfo.descSetLayout);
    if (pushSize) {
        layoutInfo.setPushConstantRanges(pushRange);
    }
    pipelineLayout = context->getDevice().createPipelineLayoutUnique(layoutInfo);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    if (createInfo.taskShader && createInfo.taskShader->getModule()) {
        shaderStages.resize(3);
        shaderStages[0].setModule(createInfo.taskShader->getModule());
        shaderStages[0].setStage(createInfo.taskShader->getStage());
        shaderStages[0].setPName("main");
        shaderStages[1].setModule(createInfo.meshShader->getModule());
        shaderStages[1].setStage(createInfo.meshShader->getStage());
        shaderStages[1].setPName("main");
        shaderStages[2].setModule(createInfo.fragmentShader->getModule());
        shaderStages[2].setStage(createInfo.fragmentShader->getStage());
        shaderStages[2].setPName("main");
    } else {
        shaderStages.resize(2);
        shaderStages[0].setModule(createInfo.meshShader->getModule());
        shaderStages[0].setStage(createInfo.meshShader->getStage());
        shaderStages[0].setPName("main");
        shaderStages[1].setModule(createInfo.fragmentShader->getModule());
        shaderStages[1].setStage(createInfo.fragmentShader->getStage());
        shaderStages[1].setPName("main");
    }

    // Pipeline states
    std::vector<vk::DynamicState> dynamicStates;

    vk::PipelineColorBlendAttachmentState colorBlendState;
    colorBlendState.setColorWriteMask(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    if (createInfo.alphaBlending) {
        colorBlendState.setBlendEnable(true);
        colorBlendState.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha);
        colorBlendState.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
        colorBlendState.setColorBlendOp(vk::BlendOp::eAdd);
        colorBlendState.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
        colorBlendState.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
        colorBlendState.setAlphaBlendOp(vk::BlendOp::eAdd);
    }

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.setAttachments(colorBlendState);
    colorBlending.setLogicOpEnable(VK_FALSE);

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.setViewportCount(1);
    viewportState.setScissorCount(1);
    dynamicStates.push_back(vk::DynamicState::eViewport);
    dynamicStates.push_back(vk::DynamicState::eScissor);

    vk::PipelineRasterizationStateCreateInfo rasterization;
    rasterization.setDepthClampEnable(VK_FALSE);
    rasterization.setRasterizerDiscardEnable(VK_FALSE);
    rasterization.setDepthBiasEnable(VK_FALSE);

    if (std::holds_alternative<vk::PolygonMode>(createInfo.polygonMode)) {
        rasterization.setPolygonMode(std::get<vk::PolygonMode>(createInfo.polygonMode));
    } else {
        assert(std::get<std::string>(createInfo.polygonMode) == "dynamic");
        dynamicStates.push_back(vk::DynamicState::ePolygonModeEXT);
    }

    if (std::holds_alternative<vk::FrontFace>(createInfo.frontFace)) {
        rasterization.setFrontFace(std::get<vk::FrontFace>(createInfo.frontFace));
    } else {
        assert(std::get<std::string>(createInfo.frontFace) == "dynamic");
        dynamicStates.push_back(vk::DynamicState::eFrontFace);
    }

    if (std::holds_alternative<vk::CullModeFlags>(createInfo.cullMode)) {
        rasterization.setCullMode(std::get<vk::CullModeFlags>(createInfo.cullMode));
    } else {
        assert(std::get<std::string>(createInfo.cullMode) == "dynamic");
        dynamicStates.push_back(vk::DynamicState::eCullMode);
    }

    if (std::holds_alternative<float>(createInfo.lineWidth)) {
        rasterization.setLineWidth(std::get<float>(createInfo.lineWidth));
    } else {
        assert(std::get<std::string>(createInfo.lineWidth) == "dynamic");
        dynamicStates.push_back(vk::DynamicState::eLineWidth);
    }

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
    dynamicStateInfo.setDynamicStates(dynamicStates);

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.setSampleShadingEnable(VK_FALSE);

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.setDepthTestEnable(VK_TRUE);
    depthStencil.setDepthWriteEnable(VK_TRUE);
    depthStencil.setDepthCompareOp(vk::CompareOp::eLess);
    depthStencil.setDepthBoundsTestEnable(VK_FALSE);
    depthStencil.setStencilTestEnable(VK_FALSE);

    vk::Format colorFormat = vk::Format::eB8G8R8A8Unorm;
    vk::Format depthFormat = vk::Format::eD32Sfloat;
    vk::PipelineRenderingCreateInfo renderingInfo;
    renderingInfo.setColorAttachmentFormats(colorFormat);
    renderingInfo.setDepthAttachmentFormat(depthFormat);

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.setStages(shaderStages);
    pipelineInfo.setPViewportState(&viewportState);
    pipelineInfo.setPRasterizationState(&rasterization);
    pipelineInfo.setPMultisampleState(&multisampling);
    pipelineInfo.setPDepthStencilState(&depthStencil);
    pipelineInfo.setPColorBlendState(&colorBlending);
    pipelineInfo.setLayout(*pipelineLayout);
    pipelineInfo.setSubpass(0);
    pipelineInfo.setPDynamicState(&dynamicStateInfo);
    pipelineInfo.setPNext(&renderingInfo);

    auto result = context->getDevice().createGraphicsPipelineUnique({}, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a pipeline!");
    }
    pipeline = std::move(result.value);
}

ComputePipeline::ComputePipeline(const Context* context, ComputePipelineCreateInfo createInfo)
    : Pipeline{context} {
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

RayTracingPipeline::RayTracingPipeline(const Context* context,
                                       RayTracingPipelineCreateInfo createInfo)
    : Pipeline{context} {
    shaderStageFlags =
        vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
        vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR |
        vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eCallableKHR;
    bindPoint = vk::PipelineBindPoint::eRayTracingKHR;
    pushSize = createInfo.pushSize;

    rgenCount = createInfo.rgenShaders.size();
    missCount = createInfo.missShaders.size();
    hitCount = createInfo.chitShaders.size() + createInfo.ahitShaders.size();

    for (auto& shader : createInfo.rgenShaders) {
        uint32_t index = static_cast<uint32_t>(shaderModules.size());
        shaderModules.push_back(shader->getModule());

        shaderStages.push_back(
            {{}, vk::ShaderStageFlagBits::eRaygenKHR, shaderModules[index], "main"});

        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, index,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
    }
    for (auto& shader : createInfo.missShaders) {
        uint32_t index = static_cast<uint32_t>(shaderModules.size());
        shaderModules.push_back(shader->getModule());

        shaderStages.push_back(
            {{}, vk::ShaderStageFlagBits::eMissKHR, shaderModules.back(), "main"});

        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, index,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
    }
    for (auto& shader : createInfo.chitShaders) {
        uint32_t index = static_cast<uint32_t>(shaderModules.size());
        shaderModules.push_back(shader->getModule());
        shaderStages.push_back(
            {{}, vk::ShaderStageFlagBits::eClosestHitKHR, shaderModules.back(), "main"});

        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                                VK_SHADER_UNUSED_KHR, index, VK_SHADER_UNUSED_KHR,
                                VK_SHADER_UNUSED_KHR});
    }
    // TODO: support any hit shader

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
    auto rtProperties =
        context->getPhysicalDeviceProperties2<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    // Calculate SBT size
    uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    uint32_t handleSizeAligned = rtProperties.shaderGroupHandleAlignment;
    uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
    size_t sbtSize = groupCount * handleSizeAligned;

    // Get shader group handles
    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    vk::Result result = context->getDevice().getRayTracingShaderGroupHandlesKHR(
        *pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to get ray tracing shader group handles.");
    }

    // Create shader binding table
    raygenSBT = context->createBuffer({
        .usage = BufferUsage::ShaderBindingTable,
        .memory = MemoryUsage::Device,
        .size = handleSize * rgenCount,
    });
    missSBT = context->createBuffer({
        .usage = BufferUsage::ShaderBindingTable,
        .memory = MemoryUsage::Device,
        .size = handleSize * missCount,
    });
    hitSBT = context->createBuffer({
        .usage = BufferUsage::ShaderBindingTable,
        .memory = MemoryUsage::Device,
        .size = handleSize * hitCount,
    });
    context->oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
        commandBuffer->copyBuffer(  // break
            raygenSBT, shaderHandleStorage.data() + 0 * handleSizeAligned);
        commandBuffer->copyBuffer(  // break
            missSBT, shaderHandleStorage.data() + rgenCount * handleSizeAligned);
        commandBuffer->copyBuffer(  // break
            hitSBT, shaderHandleStorage.data() + (rgenCount + missCount) * handleSizeAligned);
    });

    raygenRegion.setDeviceAddress(raygenSBT->getAddress());
    raygenRegion.setStride(rtProperties.shaderGroupHandleAlignment);
    raygenRegion.setSize(rtProperties.shaderGroupHandleAlignment);

    missRegion.setDeviceAddress(missSBT->getAddress());
    missRegion.setStride(rtProperties.shaderGroupHandleAlignment);
    missRegion.setSize(rtProperties.shaderGroupHandleAlignment);

    hitRegion.setDeviceAddress(hitSBT->getAddress());
    hitRegion.setStride(rtProperties.shaderGroupHandleAlignment);
    hitRegion.setSize(rtProperties.shaderGroupHandleAlignment);
}
}  // namespace rv
