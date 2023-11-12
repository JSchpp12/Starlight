#pragma once 

#include "StarShader.hpp"
#include "StarDevice.hpp"
#include "VulkanVertex.hpp"
#include "Compiler.hpp"
#include "StarPipeline.hpp"

#include <vulkan/vulkan.hpp>

#include <iostream> 

namespace star {
class StarGraphicsPipeline : public StarPipeline{
public:
	struct PipelineConfigSettings {
		PipelineConfigSettings() = default;
		//no copy 
		PipelineConfigSettings(const PipelineConfigSettings&) = delete;
		PipelineConfigSettings& operator=(const PipelineConfigSettings&) = delete;

		vk::Rect2D scissor; 
		vk::PipelineViewportStateCreateInfo viewportInfo;
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
		vk::PipelineMultisampleStateCreateInfo multisampleInfo;
		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<VkDynamicState> dynamicStateEnables;
		vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
		vk::PipelineLayout pipelineLayout = nullptr;
		vk::RenderPass renderPass = nullptr;
		vk::Extent2D swapChainExtent; 
		uint32_t subpass = 0;
	};

	StarGraphicsPipeline(StarDevice& device, StarShader inVertShader, StarShader inFragShader, PipelineConfigSettings& configSettings);
	virtual ~StarGraphicsPipeline();

	//no copy
	StarGraphicsPipeline(const StarGraphicsPipeline&) = delete;
	StarGraphicsPipeline& operator=(const StarGraphicsPipeline&) = delete;

	virtual void bind(vk::CommandBuffer& commandBuffer) override;

	static void defaultPipelineConfigInfo(PipelineConfigSettings& configSettings, vk::Extent2D swapChainExtent, vk::RenderPass renderPass, vk::PipelineLayout pipelineLayout);

protected:
	StarShader vertShader, fragShader; 
	PipelineConfigSettings& configSettings; 

	virtual vk::Pipeline buildPipeline() override; 

};
}