#pragma once

#include "SceneRenderer.hpp"
#include "CommandBufferModifier.hpp"
#include "RenderingTargetInfo.hpp"

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace star {
	class SwapChainRenderer : public SceneRenderer{
	public:
		//how many frames will be sent through the pipeline
		const int MAX_FRAMES_IN_FLIGHT = 2;

		SwapChainRenderer(StarWindow& window, std::vector<std::unique_ptr<Light>>& lightList, 
			std::vector<std::reference_wrapper<StarObject>> objectList, 
			StarCamera& camera, 
			StarDevice& device); 

		virtual ~SwapChainRenderer();

		virtual void prepare(); 

		void submitPresentation(const int& frameIndexToBeDrawn, const vk::Semaphore* mainGraphicsDoneSemaphore); 

		void pollEvents();

		vk::Extent2D getMainExtent() const { return this->swapChainExtent; }
		int getFrameToBeDrawn() const { return this->currentFrame; }
	protected:
		StarWindow& window;

		//tracker for which frame is being processed of the available permitted frames
		int currentFrame = 0;
		int previousFrame = 0;

		uint32_t currentSwapChainImageIndex = 0;

		bool frameBufferResized = false; //explicit declaration of resize, used if driver does not trigger VK_ERROR_OUT_OF_DATE

		vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

		//Look through givent present modes and pick the "best" one
		vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

		vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
		
		virtual void prepareForSubmission(const int& frameIndexToBeDrawn) override;

		virtual void submissionDone();

		void submitBuffer(StarCommandBuffer& buffer, const int& frameIndexToBeDrawn);

		virtual std::optional<std::function<void(const int&)>> getAfterBufferSubmissionCallback() override;

		std::optional<std::function<void(StarCommandBuffer&, const int&)>> getOverrideBufferSubmissionCallback() override;
		
		virtual std::vector<vk::Image> createRenderToImages() override;

		virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoColorAttachment(const int& frameInFlightIndex) override;

		virtual void recordCommandBuffer(vk::CommandBuffer& buffer, const int& frameIndexToBeDrawn) override;
	private:
		//more swapchain info 
		vk::SwapchainKHR swapChain;

		vk::Format swapChainImageFormat;
		vk::Extent2D swapChainExtent;

		std::vector<vk::Fence> inFlightFences;
		std::vector<vk::Fence> imagesInFlight;

		/// <summary>
		/// If the swapchain is no longer compatible, it must be recreated.
		/// </summary>
		virtual void recreateSwapChain();

		void cleanupSwapChain();

		/// <summary>
		/// Create semaphores that are going to be used to sync rendering and presentation queues
		/// </summary>
		void createSemaphores();

		/// <summary>
		/// Fences are needed for CPU-GPU sync. Creates these required objects
		/// </summary>
		void createFences();

		/// <summary>
		/// Create tracking information in order to link fences with the swap chain images using 
		/// </summary>
		void createFenceImageTracking();

		/// <summary>
		/// Create a swap chain that will be used in rendering images
		/// </summary>
		virtual void createSwapChain();
	};
}