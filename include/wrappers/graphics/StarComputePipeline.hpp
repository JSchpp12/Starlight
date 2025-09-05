#pragma once

#include "StarPipeline.hpp"
#include "Handle.hpp"

namespace star {
	class StarComputePipeline : public StarPipeline {
	public:
		StarComputePipeline(vk::PipelineLayout pipelineLayout, StarShader inCompShader); 

		// Inherited via StarPipeline
		void bind(vk::CommandBuffer& commandBuffer) override;
	protected:
		StarShader compShader; 
		vk::PipelineLayout pipelineLayout; //Compute pipes have their own layout -- much smaller than graphics 

		vk::Pipeline buildPipeline(core::device::DeviceContext &context) override;

		std::vector<Handle> submitShaders(core::device::DeviceContext &context) override; 
	};
}