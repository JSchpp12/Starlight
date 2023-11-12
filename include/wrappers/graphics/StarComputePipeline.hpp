#pragma once

#include "StarPipeline.hpp"

namespace star {
	class StarComputePipeline : public StarPipeline {
	public:
		// Inherited via StarPipeline
		void bind(vk::CommandBuffer& commandBuffer) override;

		vk::Pipeline buildPipeline() override;

	};
}