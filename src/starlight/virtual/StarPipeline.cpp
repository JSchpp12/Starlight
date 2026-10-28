#include "StarPipeline.hpp"

#include "starlight/core/Exceptions.hpp"

#include <cassert>
#include <stdexcept>

namespace star
{

void StarPipeline::bind(vk::CommandBuffer &commandBuffer) const
{
    assert(m_pipeline && "Pipeline has not yet been created");

    const vk::PipelineBindPoint bindPoint =
        (m_type == PipelineType::Graphics) ? vk::PipelineBindPoint::eGraphics : vk::PipelineBindPoint::eCompute;
    commandBuffer.bindPipeline(bindPoint, m_pipeline);
}

void StarPipeline::destroy(vk::Device device)
{
    if (m_pipeline)
    {
        device.destroyPipeline(m_pipeline);
        m_pipeline = VK_NULL_HANDLE;
    }
}

PipelineProvider::Builder &PipelineProvider::Builder::setShaders(std::vector<Handle> shaders)
{
    m_type = PipelineType::Graphics;
    m_shaders = std::move(shaders);
    return *this;
}

PipelineProvider::Builder &PipelineProvider::Builder::setComputeShader(Handle shader)
{
    m_type = PipelineType::Compute;
    m_shaders = {std::move(shader)};
    return *this;
}

PipelineProvider::Builder &PipelineProvider::Builder::setPipelineLayout(vk::PipelineLayout layout)
{
    m_layout = layout;
    return *this;
}

PipelineProvider::Builder &PipelineProvider::Builder::setGraphicsOverrides(GraphicsOverrides overrides)
{
    m_graphics = std::move(overrides);
    return *this;
}

PipelineProvider::Builder &PipelineProvider::Builder::setPipelineConstantsForStage(Shader_Stage stage,
                                                                                   StarPipelineConstants constants)
{
    assert(!m_pipelineConstants.contains(stage) && "Pipeline constants for this stage have already been set");
    m_pipelineConstants[stage] = std::move(constants);
    return *this;
}

PipelineProvider PipelineProvider::Builder::build()
{
    PipelineProvider provider;
    provider.m_type = m_type;
    provider.m_shaders = std::move(m_shaders);
    provider.m_layout = m_layout;
    provider.m_graphics = std::move(m_graphics);
    provider.m_pipelineConstants = std::move(m_pipelineConstants);
    return provider;
}

static Shader_Stage ToShaderStage(vk::ShaderStageFlagBits stage)
{
    switch (stage)
    {
    case vk::ShaderStageFlagBits::eVertex:
        return Shader_Stage::vertex;
    case vk::ShaderStageFlagBits::eFragment:
        return Shader_Stage::fragment;
    case vk::ShaderStageFlagBits::eCompute:
        return Shader_Stage::compute;
    default:
        assert(false && "Unsupported shader stage");
        STAR_THROW("Unsupported shader stage");
    }
}

PipelineProvider::SpecializedStages PipelineProvider::processPipelineConstants(
    const std::vector<vk::PipelineShaderStageCreateInfo> &workingShaderInfos)
{
    SpecializedStages result;
    result.shaderStages = workingShaderInfos;
    result.specializationInfos.reserve(workingShaderInfos.size());

    for (size_t i = 0; i < workingShaderInfos.size(); ++i)
    {
        // Process each shader stage for specialization constants if needed
        if (m_pipelineConstants.contains(ToShaderStage(workingShaderInfos[i].stage)))
        {
            result.specializationInfos.push_back(
                m_pipelineConstants[ToShaderStage(workingShaderInfos[i].stage)].getSpecializationInfo());
            result.shaderStages[i].pSpecializationInfo = &result.specializationInfos.back();
        }
    }
    return result;
}

StarPipeline PipelineProvider::createPipeline(vk::Device device, RenderResourceDependencies &deps)
{
    vk::Pipeline pipeline = (m_type == PipelineType::Graphics)
                                ? buildGraphics(device, deps, processPipelineConstants(deps.shaderStages))
                                : buildCompute(device, deps, processPipelineConstants(deps.shaderStages));
    return StarPipeline{pipeline, m_type};
}

vk::Pipeline PipelineProvider::buildGraphics(vk::Device device, RenderResourceDependencies &deps,
                                             const SpecializedStages &specializedStages)
{
    // ---- Vertex input: provider overrides, falling back to the global VulkanVertex ----
    std::vector<vk::VertexInputBindingDescription> vertexBindings = m_graphics.vertexInput.bindings;
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes = m_graphics.vertexInput.attributes;

    if (vertexBindings.empty())
    {
        vertexBindings = {VulkanVertex::getBindingDescription()};
    }
    if (vertexAttributes.empty())
    {
        auto defaultAttributes = VulkanVertex::getAttributeDescriptions();
        vertexAttributes.assign(defaultAttributes.begin(), defaultAttributes.end());
    }

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

    vk::PipelineRenderingCreateInfoKHR renderingCreateInfo{};
    renderingCreateInfo.pNext = VK_NULL_HANDLE;
    renderingCreateInfo.sType = vk::StructureType::ePipelineRenderingCreateInfoKHR;
    renderingCreateInfo.colorAttachmentCount = deps.renderingTargetInfo.colorAttachmentFormats.size();
    renderingCreateInfo.pColorAttachmentFormats = deps.renderingTargetInfo.colorAttachmentFormats.size() != 0
                                                      ? deps.renderingTargetInfo.colorAttachmentFormats.data()
                                                      : nullptr;
    if (deps.renderingTargetInfo.depthAttachmentFormat.has_value())
        renderingCreateInfo.depthAttachmentFormat = deps.renderingTargetInfo.depthAttachmentFormat.value();
    if (deps.renderingTargetInfo.stencilAttachmentFormat.has_value())
        renderingCreateInfo.stencilAttachmentFormat = deps.renderingTargetInfo.stencilAttachmentFormat.value();

    // ---- Engine default state, then apply provider overrides ----
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
    inputAssembly.topology = m_graphics.topology.value_or(vk::PrimitiveTopology::eTriangleList);
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = m_graphics.cullMode.value_or(vk::CullModeFlagBits::eBack);
    rasterizer.frontFace = m_graphics.frontFace.value_or(vk::FrontFace::eCounterClockwise);
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = vk::StructureType::ePipelineDepthStencilStateCreateInfo;
    if (m_graphics.depthStencil.has_value())
    {
        depthStencil = m_graphics.depthStencil.value();
    }
    else
    {
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = vk::CompareOp::eLess;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
    }

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    if (m_graphics.colorBlendAttachment.has_value())
    {
        colorBlendAttachment = m_graphics.colorBlendAttachment.value();
    }
    else
    {
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorBlendAttachment.blendEnable = VK_FALSE;
    }

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    /* Dynamic State */
    std::vector<vk::DynamicState> dynamicStates = m_graphics.dynamicStates;
    if (dynamicStates.empty())
    {
        dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eLineWidth};
    }
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{0, 0};
    scissor.extent = deps.swapChainExtent;

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
    viewportState.viewportCount = 1;
    viewportState.pViewports = VK_NULL_HANDLE;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    /* Pipeline */
    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    pipelineInfo.stageCount = static_cast<uint32_t>(specializedStages.shaderStages.size());
    pipelineInfo.pStages = specializedStages.shaderStages.data();

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = m_layout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.pNext = &renderingCreateInfo;

    auto result = device.createGraphicsPipelines(VK_NULL_HANDLE, pipelineInfo);
    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }
    if (result.value.size() > 1)
    {
        throw std::runtime_error("unknown error has occurred, more than one pipeline was created ");
    }

    return result.value.at(0);
}

vk::Pipeline PipelineProvider::buildCompute(vk::Device device, RenderResourceDependencies &deps,
                                            const SpecializedStages &specializedStages)
{
    assert(deps.shaderStages.size() == 1 && "More shaders than expected for a compute pipeline");
    assert(deps.shaderStages.at(0).stage == vk::ShaderStageFlagBits::eCompute &&
           "Shaders for a compute pipeline should be compute shaders");

    vk::ComputePipelineCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eComputePipelineCreateInfo;
    createInfo.layout = m_layout;
    createInfo.stage = specializedStages.shaderStages[0];

    auto result = device.createComputePipeline(VK_NULL_HANDLE, createInfo);
    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create compute pipeline");
    }

    return result.value;
}

} // namespace star