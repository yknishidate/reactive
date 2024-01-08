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

    // Raygen
    {
        uint32_t rgenIndex = static_cast<uint32_t>(shaderModules.size());
        rgenCount = 1;
        shaderModules.push_back(createInfo.rgenGroup.raygenShader->getModule());
        shaderStages.push_back(
            {{}, vk::ShaderStageFlagBits::eRaygenKHR, shaderModules.back(), "main"});
        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, rgenIndex,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
    }

    // Miss
    for (auto& group : createInfo.missGroups) {
        auto& shader = group.missShader;
        uint32_t missIndex = static_cast<uint32_t>(shaderModules.size());
        missCount += 1;
        shaderModules.push_back(shader->getModule());
        shaderStages.push_back(
            {{}, vk::ShaderStageFlagBits::eMissKHR, shaderModules.back(), "main"});
        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, missIndex,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
    }

    // Hit
    for (auto& group : createInfo.hitGroups) {
        auto& chitShader = group.chitShader;
        auto& ahitShader = group.ahitShader;

        uint32_t chitIndex = VK_SHADER_UNUSED_KHR;
        uint32_t ahitIndex = VK_SHADER_UNUSED_KHR;
        if (chitShader) {
            chitIndex = static_cast<uint32_t>(shaderModules.size());
            hitCount += 1;
            shaderModules.push_back(chitShader->getModule());
            shaderStages.push_back(
                {{}, vk::ShaderStageFlagBits::eClosestHitKHR, shaderModules.back(), "main"});
        }
        if (ahitShader) {
            ahitIndex = static_cast<uint32_t>(shaderModules.size());
            hitCount += 1;
            shaderModules.push_back(ahitShader->getModule());
            shaderStages.push_back(
                {{}, vk::ShaderStageFlagBits::eAnyHitKHR, shaderModules.back(), "main"});
        }
        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                                VK_SHADER_UNUSED_KHR, chitIndex, ahitIndex, VK_SHADER_UNUSED_KHR});
    }

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

    createSBT();
}

void RayTracingPipeline::createSBT() {
    // Get Ray Tracing Properties
    auto rtProperties =
        context->getPhysicalDeviceProperties2<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    auto alignUp = [&](uint32_t size, uint32_t alignment) -> uint32_t {
        return (size + alignment - 1) & ~(alignment - 1);
    };

    // Calculate SBT size
    uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    uint32_t handleAlignment = rtProperties.shaderGroupHandleAlignment;
    uint32_t baseAlignment = rtProperties.shaderGroupBaseAlignment;
    uint32_t handleSizeAligned = alignUp(handleSize, handleAlignment);

    raygenRegion.setStride(alignUp(handleSizeAligned, baseAlignment));
    raygenRegion.setSize(raygenRegion.stride);

    missRegion.setStride(handleSizeAligned);
    missRegion.setSize(alignUp(missCount * handleSizeAligned, baseAlignment));

    hitRegion.setStride(handleSizeAligned);
    hitRegion.setSize(alignUp(hitCount * handleSizeAligned, baseAlignment));

    // Create shader binding table
    vk::DeviceSize sbtSize = raygenRegion.size + missRegion.size + hitRegion.size;
    sbtBuffer = context->createBuffer({
        .usage = BufferUsage::ShaderBindingTable,
        .memory = MemoryUsage::Host,
        .size = sbtSize,
    });

    // Get shader group handles
    uint32_t handleCount = rgenCount + missCount + hitCount;
    uint32_t handleStorageSize = handleCount * handleSize;
    std::vector<uint8_t> handleStorage(handleStorageSize);
    auto result = context->getDevice().getRayTracingShaderGroupHandlesKHR(
        *pipeline, 0, handleCount, handleStorageSize, handleStorage.data());
    if (result != vk::Result::eSuccess) {
        std::cerr << "Failed to get ray tracing shader group handles.\n";
        std::abort();
    }

    // Copy handles
    uint8_t* sbtHead = static_cast<uint8_t*>(sbtBuffer->map());
    uint8_t* dstPtr = sbtHead;
    auto copyHandle = [&](uint32_t index) {
        std::memcpy(dstPtr, handleStorage.data() + handleSize * index, handleSize);
    };

    // Raygen
    uint32_t handleIndex = 0;
    copyHandle(handleIndex++);

    // Miss
    dstPtr = sbtHead + raygenRegion.size;
    for (uint32_t c = 0; c < missCount; c++) {
        copyHandle(handleIndex++);
        dstPtr += missRegion.stride;
    }

    // Hit
    dstPtr = sbtHead + raygenRegion.size + missRegion.size;
    for (uint32_t c = 0; c < hitCount; c++) {
        copyHandle(handleIndex++);
        dstPtr += hitRegion.stride;
    }

    raygenRegion.setDeviceAddress(sbtBuffer->getAddress());
    missRegion.setDeviceAddress(sbtBuffer->getAddress() + raygenRegion.size);
    hitRegion.setDeviceAddress(sbtBuffer->getAddress() + raygenRegion.size + missRegion.size);
}
}  // namespace rv
