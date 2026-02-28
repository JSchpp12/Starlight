// #include "StarGraphicsPipeline.hpp"

// namespace star
// {

// void StarGraphicsPipeline::bind(vk::CommandBuffer &commandBuffer)
// {
//     commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
// }

// void StarGraphicsPipeline::defaultPipelineConfigInfo(PipelineConfigSettings &configSettings,
//                                                      vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout,
//                                                      core::renderer::RenderingTargetInfo renderingInfo)
// {

// }

// void ProcessShaders(vk::Device &device, const StarPipeline::RenderResourceDepdencies &deps, vk::ShaderModule &vertModule, vk::ShaderModule &fragModule, vk::ShaderModule &geoModule){
//     for (const auto& shader : deps.compiledShaders){
//         switch(shader.first.getStage()){
//             case(Shader_Stage::vertex):
//             {
//                 vertModule = StarPipeline::CreateShaderModule(device, *shader.second); 
//                 break;
//             }
//             case(Shader_Stage::fragment):
//             {
//                 fragModule = StarPipeline::CreateShaderModule(device, *shader.second); 
//                 break;
//             }
//             case(Shader_Stage::geometry):
//             {
//                 geoModule = StarPipeline::CreateShaderModule(device, *shader.second); 
//                 break;
//             }
//             default:
//             throw std::runtime_error("Unsupported shader stage"); 
//         }
//     }
// }

// vk::Pipeline StarGraphicsPipeline::BuildPipeline(vk::Device device, const vk::PipelineLayout &pipelineLayout, const StarPipeline::RenderResourceDepdencies &deps)
// {

// }

// std::vector<Handle> StarGraphicsPipeline::submitShaders(core::device::DeviceContext &context)
// {
//     std::vector<Handle> shaders{context.getShaderManager().submit(vertShader),
//                                 context.getShaderManager().submit(fragShader)};

//     if (geomShader.has_value())
//     {
//         shaders.push_back(context.getShaderManager().submit(geomShader.value()));
//     }
//     return shaders;
// }
// } // namespace star