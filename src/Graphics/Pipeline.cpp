#include "reactive/Graphics/Pipeline.hpp"

#include <regex>

#include "reactive/Compiler/Compiler.hpp"
#include "reactive/Graphics/ArrayProxy.hpp"
#include "reactive/Graphics/Buffer.hpp"
#include "reactive/Graphics/CommandBuffer.hpp"
#include "reactive/Scene/Mesh.hpp"
#include "reactive/Scene/Object.hpp"

namespace rv {
GraphicsPipeline::GraphicsPipeline(const Context& context,
                                   const GraphicsPipelineCreateInfo& createInfo)
    : Pipeline{context} {
    m_shaderStageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    m_bindPoint = vk::PipelineBindPoint::eGraphics;
    m_pushSize = createInfo.pushSize;

    vk::PushConstantRange pushRange;
    pushRange.setOffset(0);
    pushRange.setSize(createInfo.pushSize);
    pushRange.setStageFlags(m_shaderStageFlags);

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.setSetLayouts(createInfo.descSetLayout);
    if (m_pushSize) {
        layoutInfo.setPushConstantRanges(pushRange);
    }
    m_pipelineLayout = m_context->getDevice().createPipelineLayoutUnique(layoutInfo);

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
    pipelineInfo.setLayout(*m_pipelineLayout);
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

    auto result = m_context->getDevice().createGraphicsPipelineUnique({}, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a m_pipeline!");
    }
    m_pipeline = std::move(result.value);
}

MeshShaderPipeline::MeshShaderPipeline(const Context& context,
                                       const MeshShaderPipelineCreateInfo& createInfo)
    : Pipeline{context} {
    m_shaderStageFlags = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT |
                       vk::ShaderStageFlagBits::eFragment;
    m_bindPoint = vk::PipelineBindPoint::eGraphics;
    m_pushSize = createInfo.pushSize;

    vk::PushConstantRange pushRange;
    pushRange.setOffset(0);
    pushRange.setSize(createInfo.pushSize);
    pushRange.setStageFlags(m_shaderStageFlags);

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.setSetLayouts(createInfo.descSetLayout);
    if (m_pushSize) {
        layoutInfo.setPushConstantRanges(pushRange);
    }
    m_pipelineLayout = m_context->getDevice().createPipelineLayoutUnique(layoutInfo);

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

    vk::PipelineRenderingCreateInfo renderingInfo;
    renderingInfo.setColorAttachmentFormats(createInfo.colorFormats);
    renderingInfo.setDepthAttachmentFormat(createInfo.depthFormat);

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.setStages(shaderStages);
    pipelineInfo.setPViewportState(&viewportState);
    pipelineInfo.setPRasterizationState(&rasterization);
    pipelineInfo.setPMultisampleState(&multisampling);
    pipelineInfo.setPDepthStencilState(&depthStencil);
    pipelineInfo.setPColorBlendState(&colorBlending);
    pipelineInfo.setLayout(*m_pipelineLayout);
    pipelineInfo.setSubpass(0);
    pipelineInfo.setPDynamicState(&dynamicStateInfo);
    pipelineInfo.setPNext(&renderingInfo);

    auto result = m_context->getDevice().createGraphicsPipelineUnique({}, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a m_pipeline!");
    }
    m_pipeline = std::move(result.value);
}

ComputePipeline::ComputePipeline(const Context& context,
                                 const ComputePipelineCreateInfo& createInfo)
    : Pipeline{context} {
    m_shaderStageFlags = vk::ShaderStageFlagBits::eCompute;
    m_bindPoint = vk::PipelineBindPoint::eCompute;
    m_pushSize = createInfo.pushSize;

    vk::PushConstantRange pushRange;
    pushRange.setOffset(0);
    pushRange.setSize(m_pushSize);
    pushRange.setStageFlags(m_shaderStageFlags);

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.setSetLayouts(createInfo.descSetLayout);
    if (m_pushSize) {
        layoutInfo.setPushConstantRanges(pushRange);
    }
    m_pipelineLayout = m_context->getDevice().createPipelineLayoutUnique(layoutInfo);

    vk::PipelineShaderStageCreateInfo stage;
    stage.setStage(createInfo.computeShader->getStage());
    stage.setModule(createInfo.computeShader->getModule());
    stage.setPName("main");

    vk::ComputePipelineCreateInfo pipelineInfo;
    pipelineInfo.setStage(stage);
    pipelineInfo.setLayout(*m_pipelineLayout);
    auto res = m_context->getDevice().createComputePipelinesUnique({}, pipelineInfo);
    if (res.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a m_pipeline.");
    }
    m_pipeline = std::move(res.value.front());
}

RayTracingPipeline::RayTracingPipeline(const Context& context,
                                       const RayTracingPipelineCreateInfo& createInfo)
    : Pipeline{context} {
    m_shaderStageFlags =
        vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
        vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR |
        vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eCallableKHR;
    m_bindPoint = vk::PipelineBindPoint::eRayTracingKHR;
    m_pushSize = createInfo.pushSize;

    // Raygen
    {
        uint32_t rgenIndex = static_cast<uint32_t>(m_shaderModules.size());
        m_rgenCount = 1;
        m_shaderModules.push_back(createInfo.rgenGroup.raygenShader->getModule());
        m_shaderStages.push_back(
            {{}, vk::ShaderStageFlagBits::eRaygenKHR, m_shaderModules.back(), "main"});
        m_shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, rgenIndex,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
    }

    // Miss
    for (auto& group : createInfo.missGroups) {
        auto& shader = group.missShader;
        uint32_t missIndex = static_cast<uint32_t>(m_shaderModules.size());
        m_missCount += 1;
        m_shaderModules.push_back(shader->getModule());
        m_shaderStages.push_back(
            {{}, vk::ShaderStageFlagBits::eMissKHR, m_shaderModules.back(), "main"});
        m_shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, missIndex,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
    }

    // Hit
    for (auto& group : createInfo.hitGroups) {
        auto& chitShader = group.chitShader;
        auto& ahitShader = group.ahitShader;

        uint32_t chitIndex = VK_SHADER_UNUSED_KHR;
        uint32_t ahitIndex = VK_SHADER_UNUSED_KHR;
        if (chitShader) {
            chitIndex = static_cast<uint32_t>(m_shaderModules.size());
            m_hitCount += 1;
            m_shaderModules.push_back(chitShader->getModule());
            m_shaderStages.push_back(
                {{}, vk::ShaderStageFlagBits::eClosestHitKHR, m_shaderModules.back(), "main"});
        }
        if (ahitShader) {
            ahitIndex = static_cast<uint32_t>(m_shaderModules.size());
            m_hitCount += 1;
            m_shaderModules.push_back(ahitShader->getModule());
            m_shaderStages.push_back(
                {{}, vk::ShaderStageFlagBits::eAnyHitKHR, m_shaderModules.back(), "main"});
        }
        m_shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                                VK_SHADER_UNUSED_KHR, chitIndex, ahitIndex, VK_SHADER_UNUSED_KHR});
    }

    vk::PushConstantRange pushRange;
    pushRange.setOffset(0);
    pushRange.setSize(m_pushSize);
    pushRange.setStageFlags(m_shaderStageFlags);

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.setSetLayouts(createInfo.descSetLayout);
    if (m_pushSize) {
        layoutInfo.setPushConstantRanges(pushRange);
    }
    m_pipelineLayout = m_context->getDevice().createPipelineLayoutUnique(layoutInfo);

    vk::RayTracingPipelineCreateInfoKHR pipelineInfo;
    pipelineInfo.setStages(m_shaderStages);
    pipelineInfo.setGroups(m_shaderGroups);
    pipelineInfo.setMaxPipelineRayRecursionDepth(createInfo.maxRayRecursionDepth);
    pipelineInfo.setLayout(*m_pipelineLayout);
    auto res =
        m_context->getDevice().createRayTracingPipelineKHRUnique(nullptr, nullptr, pipelineInfo);
    if (res.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create a m_pipeline.");
    }
    m_pipeline = std::move(res.value);

    createSBT();
}

void RayTracingPipeline::createSBT() {
    // Get Ray Tracing Properties
    auto rtProperties =
        m_context->getPhysicalDeviceProperties2<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    auto alignUp = [&](uint32_t size, uint32_t alignment) -> uint32_t {
        return (size + alignment - 1) & ~(alignment - 1);
    };

    // Calculate SBT size
    uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    uint32_t handleAlignment = rtProperties.shaderGroupHandleAlignment;
    uint32_t baseAlignment = rtProperties.shaderGroupBaseAlignment;
    uint32_t handleSizeAligned = alignUp(handleSize, handleAlignment);

    m_raygenRegion.setStride(alignUp(handleSizeAligned, baseAlignment));
    m_raygenRegion.setSize(m_raygenRegion.stride);

    m_missRegion.setStride(handleSizeAligned);
    m_missRegion.setSize(alignUp(m_missCount * handleSizeAligned, baseAlignment));

    m_hitRegion.setStride(handleSizeAligned);
    m_hitRegion.setSize(alignUp(m_hitCount * handleSizeAligned, baseAlignment));

    // Create shader binding table
    vk::DeviceSize sbtSize = m_raygenRegion.size + m_missRegion.size + m_hitRegion.size;
    m_sbtBuffer = m_context->createBuffer({
        .usage = BufferUsage::ShaderBindingTable,
        .memory = MemoryUsage::Host,
        .size = sbtSize,
    });

    // Get shader group handles
    uint32_t handleCount = m_rgenCount + m_missCount + m_hitCount;
    uint32_t handleStorageSize = handleCount * handleSize;
    std::vector<uint8_t> handleStorage(handleStorageSize);
    auto result = m_context->getDevice().getRayTracingShaderGroupHandlesKHR(
        *m_pipeline, 0, handleCount, handleStorageSize, handleStorage.data());
    if (result != vk::Result::eSuccess) {
        std::cerr << "Failed to get ray tracing m_shader group handles.\n";
        std::abort();
    }

    // Copy handles
    uint8_t* sbtHead = static_cast<uint8_t*>(m_sbtBuffer->map());
    uint8_t* dstPtr = sbtHead;
    auto copyHandle = [&](uint32_t index) {
        std::memcpy(dstPtr, handleStorage.data() + handleSize * index, handleSize);
    };

    // Raygen
    uint32_t handleIndex = 0;
    copyHandle(handleIndex++);

    // Miss
    dstPtr = sbtHead + m_raygenRegion.size;
    for (uint32_t c = 0; c < m_missCount; c++) {
        copyHandle(handleIndex++);
        dstPtr += m_missRegion.stride;
    }

    // Hit
    dstPtr = sbtHead + m_raygenRegion.size + m_missRegion.size;
    for (uint32_t c = 0; c < m_hitCount; c++) {
        copyHandle(handleIndex++);
        dstPtr += m_hitRegion.stride;
    }

    m_raygenRegion.setDeviceAddress(m_sbtBuffer->getAddress());
    m_missRegion.setDeviceAddress(m_sbtBuffer->getAddress() + m_raygenRegion.size);
    m_hitRegion.setDeviceAddress(m_sbtBuffer->getAddress() + m_raygenRegion.size + m_missRegion.size);
}
}  // namespace rv
