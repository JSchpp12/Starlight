#pragma once

#include "StarShader.hpp"
#include "device/StarDevice.hpp"

#include "vulkan/vulkan.hpp"

#include <vector>
#include <utility>

namespace star {
	class StarSystemPipeline {
	public:
		StarSystemPipeline(std::vector<StarShader> definedShaders); 
		~StarSystemPipeline() = default; 

		virtual void init(StarDevice& device)=0;

		virtual void recordCommands(vk::CommandBuffer& commandBuffer, int swapChainImageIndex)=0; 


	protected:
		std::vector<StarShader> shaders; 

	};
}