#pragma once

#include "StarPipeline.hpp"
#include "StarShader.hpp"

namespace star {
	class StarComputePipeline : public StarPipeline {
	public:
		StarComputePipeline(core::device::DeviceContext& device, vk::PipelineLayout& pipelineLayout, StarShader inCompShader); 

		// Inherited via StarPipeline
		void bind(vk::CommandBuffer& commandBuffer) override;

		vk::Pipeline buildPipeline() override;

	protected:
		StarShader compShader; 
		vk::PipelineLayout& pipelineLayout; //Compute pipes have their own layout -- much smaller than graphics 
	};
}