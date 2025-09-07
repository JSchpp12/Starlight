#include "StarComputePipeline.hpp"

star::StarComputePipeline::StarComputePipeline(vk::PipelineLayout pipelineLayout, StarShader inCompShader)
    : compShader(std::move(inCompShader)), pipelineLayout(std::move(pipelineLayout))
{
}

void star::StarComputePipeline::bind(vk::CommandBuffer &commandBuffer)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);
}

vk::Pipeline star::StarComputePipeline::buildPipeline(core::device::DeviceContext &context)
{
    vk::ShaderModule compShaderModule;
    {
        // auto compiledCode = std::move(context.getShaderManager().get(getShaders()[0]).compiledShader);
        auto compiledCode = Compiler::compile(compShader.getPath(), false); 
        compShaderModule = createShaderModule(context.getDevice(), *compiledCode);
    }

    vk::PipelineShaderStageCreateInfo compShaderStageInfo{};
    compShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    compShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
    compShaderStageInfo.module = compShaderModule;
    compShaderStageInfo.pName = "main";

    vk::ComputePipelineCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eComputePipelineCreateInfo;
    createInfo.layout = this->pipelineLayout;
    createInfo.stage = compShaderStageInfo;

    auto result = context.getDevice().getVulkanDevice().createComputePipeline(VK_NULL_HANDLE, createInfo);
    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create compute pipeline");
    }

    context.getDevice().getVulkanDevice().destroyShaderModule(compShaderModule);

    return result.value;
}

std::vector<star::Handle> star::StarComputePipeline::submitShaders(core::device::DeviceContext &context)
{
    return std::vector<star::Handle>{context.getShaderManager().submit(compShader)};
}