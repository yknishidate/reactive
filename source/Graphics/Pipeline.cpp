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
    pushRange.setSize(createInfo.pushSize);
    pushRange.setStageFlags(shaderStageFlags);

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.setSetLayouts(createInfo.descSetLayout);
    if (pushSize) {
        layoutInfo.setPushConstantRanges(pushRange);
    }
    pipelineLayout = context->getDevice().createPipelineLayoutUnique(layoutInfo);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages(2);
    shaderStages[0].setModule(createInfo.vertexShader.getModule());
    shaderStages[0].setStage(createInfo.vertexShader.getStage());
    shaderStages[0].setPName("main");
    shaderStages[1].setModule(createInfo.fragmentShader.getModule());
    shaderStages[1].setStage(createInfo.fragmentShader.getStage());
    shaderStages[1].setPName("main");

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
    if (std::holds_alternative<vk::Viewport>(createInfo.viewport)) {
        viewportState.setViewports(std::get<vk::Viewport>(createInfo.viewport));
    } else {
        assert(std::get<std::string>(createInfo.viewport) == "dynamic");
        viewportState.setViewportCount(1);
        dynamicStates.push_back(vk::DynamicState::eViewport);
    }

    if (std::holds_alternative<vk::Rect2D>(createInfo.scissor)) {
        viewportState.setScissors(std::get<vk::Rect2D>(createInfo.scissor));
    } else {
        assert(std::get<std::string>(createInfo.scissor) == "dynamic");
        viewportState.setScissorCount(1);
        dynamicStates.push_back(vk::DynamicState::eScissor);
    }

    vk::PipelineRasterizationStateCreateInfo rasterization;
    rasterization.setDepthClampEnable(VK_FALSE);
    rasterization.setRasterizerDiscardEnable(VK_FALSE);
    rasterization.setPolygonMode(createInfo.polygonMode);
    rasterization.setDepthBiasEnable(VK_FALSE);

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
    if (createInfo.taskShader.getModule()) {
        shaderStages.resize(3);
        shaderStages[0].setModule(createInfo.taskShader.getModule());
        shaderStages[0].setStage(createInfo.taskShader.getStage());
        shaderStages[0].setPName("main");
        shaderStages[1].setModule(createInfo.meshShader.getModule());
        shaderStages[1].setStage(createInfo.meshShader.getStage());
        shaderStages[1].setPName("main");
        shaderStages[2].setModule(createInfo.fragmentShader.getModule());
        shaderStages[2].setStage(createInfo.fragmentShader.getStage());
        shaderStages[2].setPName("main");
    } else {
        shaderStages.resize(2);
        shaderStages[0].setModule(createInfo.meshShader.getModule());
        shaderStages[0].setStage(createInfo.meshShader.getStage());
        shaderStages[0].setPName("main");
        shaderStages[1].setModule(createInfo.fragmentShader.getModule());
        shaderStages[1].setStage(createInfo.fragmentShader.getStage());
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
    if (std::holds_alternative<vk::Viewport>(createInfo.viewport)) {
        viewportState.setViewports(std::get<vk::Viewport>(createInfo.viewport));
    } else {
        assert(std::get<std::string>(createInfo.viewport) == "dynamic");
        viewportState.setViewportCount(1);
        dynamicStates.push_back(vk::DynamicState::eViewport);
    }

    if (std::holds_alternative<vk::Rect2D>(createInfo.scissor)) {
        viewportState.setScissors(std::get<vk::Rect2D>(createInfo.scissor));
    } else {
        assert(std::get<std::string>(createInfo.scissor) == "dynamic");
        viewportState.setScissorCount(1);
        dynamicStates.push_back(vk::DynamicState::eScissor);
    }

    vk::PipelineRasterizationStateCreateInfo rasterization;
    rasterization.setDepthClampEnable(VK_FALSE);
    rasterization.setRasterizerDiscardEnable(VK_FALSE);
    rasterization.setPolygonMode(createInfo.polygonMode);
    rasterization.setDepthBiasEnable(VK_FALSE);

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
    pipelineInfo.setPDynamicState(&dynamicStateInfo);

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
    stage.setStage(createInfo.computeShader.getStage());
    stage.setModule(createInfo.computeShader.getModule());
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

    rgenCount = createInfo.rgenShaders.size();
    missCount = createInfo.missShaders.size();
    hitCount = createInfo.chitShaders.size() + createInfo.ahitShaders.size();

    for (auto& shader : createInfo.rgenShaders) {
        uint32_t index = shaderModules.size();
        shaderModules.push_back(shader.getModule());

        shaderStages.push_back(
            {{}, vk::ShaderStageFlagBits::eRaygenKHR, shaderModules[index], "main"});

        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, index,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
    }
    for (auto& shader : createInfo.missShaders) {
        uint32_t index = shaderModules.size();
        shaderModules.push_back(shader.getModule());

        shaderStages.push_back(
            {{}, vk::ShaderStageFlagBits::eMissKHR, shaderModules.back(), "main"});

        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, index,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
    }
    for (auto& shader : createInfo.chitShaders) {
        uint32_t index = shaderModules.size();
        shaderModules.push_back(shader.getModule());
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
        .data = shaderHandleStorage.data() + (rgenCount)*handleSizeAligned,
    });
    hitSBT = context->createDeviceBuffer({
        .usage = BufferUsage::ShaderBindingTable,
        .size = handleSize * hitCount,
        .data = shaderHandleStorage.data() + (rgenCount + missCount) * handleSizeAligned,
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
