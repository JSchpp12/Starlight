#pragma once 

#include "DeviceContext.hpp"

#include "vulkan/vulkan.hpp"

#include <vector>
#include <string>

namespace star {
	class StarPipeline {
	public:
		virtual ~StarPipeline();

		void init();

		virtual void bind(vk::CommandBuffer& commandBuffer)=0;

		bool areResourcesReady(); 

	protected:
		core::devices::DeviceContext& device; 
		vk::Pipeline pipeline;
		std::string hash; //this is simply the paths of all shaders in this pipeline concated together

		StarPipeline(core::devices::DeviceContext& device) : device(device) {};

		virtual vk::Pipeline buildPipeline()=0;

		bool isSame(StarPipeline& compPipe); 

		std::string getHash() { return this->hash; }

#pragma region helpers
		/// <summary>
		/// Build a shader module with the provided SPIR-V source code
		/// </summary>
		/// <param name="sourceCode"></param>
		/// <returns></returns>
		vk::ShaderModule createShaderModule(const std::vector<uint32_t>& sourceCode);
#pragma endregion
	};
}