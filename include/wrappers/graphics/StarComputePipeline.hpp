// #pragma once

// #include "Handle.hpp"
// #include "StarPipeline.hpp"


// namespace star
// {
// class StarComputePipeline : public StarPipeline
// {
//   public:
//     StarComputePipeline(StarShader inCompShader);

//     // Inherited via Pipeline
//     void bind(vk::CommandBuffer &commandBuffer) override;

//   protected:
//     StarShader compShader;

//     vk::Pipeline buildPipeline(core::device::DeviceContext &context,
//                                const vk::PipelineLayout &pipelineLayout,
//                                const core::renderer::RenderingTargetInfo &renderingInfo) override;

//     std::vector<Handle> submitShaders(core::device::DeviceContext &context) override;
// };
// } // namespace star