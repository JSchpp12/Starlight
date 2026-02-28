// #include "StarComputePipeline.hpp"

// star::StarComputePipeline::StarComputePipeline(StarShader inCompShader)
//     : compShader(std::move(inCompShader))
// {
// }

// void star::StarComputePipeline::bind(vk::CommandBuffer &commandBuffer)
// {
//     commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);
// }

// vk::Pipeline star::StarComputePipeline::buildPipeline(core::device::DeviceContext &context,
//                                                       const vk::PipelineLayout &pipelineLayout,
//                                                       const core::renderer::RenderingTargetInfo &renderingInfo)
// {
//     vk::ShaderModule compShaderModule;
//     {
//         // auto compiledCode = std::move(context.getShaderManager().get(getShaders()[0]).compiledShader);
//         auto compiledCode = Compiler::compile(compShader.getPath(), false);
//         compShaderModule = CreateShaderModule(context.getDevice(), *compiledCode);
//     }

//     vk::PipelineShaderStageCreateInfo compShaderStageInfo{};
//     compShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
//     compShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
//     compShaderStageInfo.module = compShaderModule;
//     compShaderStageInfo.pName = "main";

//     vk::ComputePipelineCreateInfo createInfo{};
//     createInfo.sType = vk::StructureType::eComputePipelineCreateInfo;
//     createInfo.layout = pipelineLayout;
//     createInfo.stage = compShaderStageInfo;

//     auto result = context.getDevice().getVulkanDevice().createComputePipeline(VK_NULL_HANDLE, createInfo);
//     if (result.result != vk::Result::eSuccess)
//     {
//         throw std::runtime_error("failed to create compute pipeline");
//     }

//     context.getDevice().getVulkanDevice().destroyShaderModule(compShaderModule);

//     return result.value;
// }

// std::vector<star::Handle> star::StarComputePipeline::submitShaders(core::device::StarDevice &device)
// {
//     return std::vector<star::Handle>{context.getShaderManager().submit(compShader)};
// }