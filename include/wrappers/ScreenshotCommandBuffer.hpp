#pragma once

#include "CommandBufferModifier.hpp"
#include "RenderResourceModifier.hpp"
#include "StarDevice.hpp"
#include "StarImage.hpp"

#include <vulkan/vulkan.hpp>
#include <vector>
#include <thread>

namespace star {
	class ScreenshotBuffer : public CommandBufferModifier, private RenderResourceModifier {

	public:
		ScreenshotBuffer(StarDevice& device, std::vector<vk::Image>& swapChainImages, 
			const vk::Extent2D& swapChainExtent, const vk::Format& swapChainImageFormat);
		~ScreenshotBuffer() = default;

		// Inherited via CommandBufferModifier
		void recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex) override;
		
		void takeScreenshot(const std::string& path); 

	protected: 
		// Inherited via RenderResourceModifier
		void initResources(StarDevice& device, const int& numFramesInFlight, 
			const vk::Extent2D& screensize) override;
		void destroyResources(StarDevice& device) override;

		// Inherited via CommandBufferModifier
		Command_Buffer_Order getCommandBufferOrder() override;
		Queue_Type getCommandBufferType() override;
		vk::PipelineStageFlags getWaitStages() override;
		bool getWillBeSubmittedEachFrame() override;
		bool getWillBeRecordedOnce() override;

		static void saveImageToDisk(); 

		static std::unique_ptr<StarImage> createNewCopyToImage(StarDevice& device, const vk::Extent2D& screensize); 

	private:
		StarDevice& device; 
		std::vector<vk::Image>& swapChainImages; 
		vk::Extent2D swapChainExtent;
		std::string screenshotSavePath; 

		std::vector<std::unique_ptr<StarImage>> copyDstImages; 
		//std::vector<vk::Image> copyDstImages;
		//std::vector<VmaAllocation> copyDstImageMemories; 

		bool supportsBlit = false; 

		bool deviceSupportsSwapchainBlit(const vk::Format& swapChainImageFormat); 

		bool deviceSupportsBlitToLinearImage(); 

		void saveScreenshotToDisk(const int& bufferIndexJustRun);
	};
}