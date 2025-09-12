// #pragma once

// #include "Compiler.hpp"
// #include "DeviceContext.hpp"
// #include "StarPipeline.hpp"
// #include "StarShader.hpp"
// #include "VulkanVertex.hpp"
// #include "core/renderer/RenderingTargetInfo.hpp"

// #include <vulkan/vulkan.hpp>

// #include <iostream>

// namespace star
// {
// class StarGraphicsPipeline : public StarPipeline
// {
//   public:
//     class Builder
//     {
//       public:
//         Builder(core::device::DeviceContext &context) : m_context(context)
//         {
//         }

//         std::unique_ptr<StarPipeline> build();

//       private:
//         core::device::DeviceContext &m_context;
//     };


//     StarGraphicsPipeline(vk::PipelineLayout layout, Handle vertShader, Handle fragShader)
//         : StarPipeline(&StarGraphicsPipeline::BuildPipeline, layout, std::vector<Handle>{vertShader, fragShader})
//     {
//     }

//     StarGraphicsPipeline(vk::PipelineLayout layout, Handle vertShader, Handle fragShader, Handle geomShader)
//         : hasGeomShader(true), StarPipeline(&StarGraphicsPipeline::BuildPipeline, layout, std::vector<Handle>{vertShader, fragShader, geomShader})
//     {
//     }

//     virtual ~StarGraphicsPipeline() = default;
//     // no copy
//     StarGraphicsPipeline(const StarGraphicsPipeline &) = delete;
//     StarGraphicsPipeline &operator=(const StarGraphicsPipeline &) = delete;
//     StarGraphicsPipeline(StarGraphicsPipeline &&) = default;
//     StarGraphicsPipeline &operator=(StarGraphicsPipeline &&) = default;

//     virtual void bind(vk::CommandBuffer &commandBuffer) override;

//     static void defaultPipelineConfigInfo(PipelineConfigSettings &configSettings, vk::Extent2D swapChainExtent,
//                                           vk::PipelineLayout pipelineLayout,
//                                           core::renderer::RenderingTargetInfo renderingInfo);

//   protected:
//     bool hasGeomShader = false;
//     PipelineConfigSettings configSettings;

//     static void ProcessShaders(vk::Device &device, const StarPipeline::RenderResourceDepdencies &deps, vk::ShaderModule &vertModule, vk::ShaderModule &fragModule, vk::ShaderModule &geoModule); 

//     static vk::Pipeline BuildPipeline(vk::Device device, const vk::PipelineLayout &pipelineLayout,
//                                        const StarPipeline::RenderResourceDepdencies &deps);

//     // virtual std::vector<Handle> submitShaders(core::device::DeviceContext &context) override;
// };
// } // namespace star