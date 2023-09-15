#pragma once

#include "StarRenderer.hpp"
#include "StarWindow.hpp"
#include "StarDescriptors.hpp"
#include "StarDevice.hpp"
#include "LightBufferObject.hpp"

#include "LightManager.hpp"

#include "Light.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

namespace star {
class BasicRenderer : public StarRenderer {
public:
	class Builder {
	public:
		Builder(StarWindow& window) : window(window) {};
		Builder& addLight(Light* light) {
			this->lightList.push_back(light);
			return *this;
		}
		std::unique_ptr<BasicRenderer> build() {
			auto newRenderer = std::unique_ptr<BasicRenderer>(new BasicRenderer(window, lightList));
			newRenderer->prepare();
			return newRenderer;
		}

	private:
		StarWindow& window;
		std::vector<Light*> lightList;

	}; 

	virtual ~BasicRenderer();

	virtual void draw() {}; 

protected:
	std::vector<Light*> lightList;

	//pipeline and dependency storage
	vk::RenderPass renderPass;

	//more swapchain info 
	vk::SwapchainKHR swapChain;
	std::vector<vk::Image> swapChainImages;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;

	std::vector<vk::ImageView> swapChainImageViews;
	std::vector<vk::Framebuffer> swapChainFramebuffers;
	std::vector<vk::Fence> inFlightFences;
	std::vector<vk::Fence> imagesInFlight;

	std::unique_ptr<StarDescriptorPool> globalPool{};
	std::unique_ptr<StarDescriptorSetLayout> globalSetLayout{};

	BasicRenderer(StarWindow& window, std::vector<Light*> inLightList);

	virtual void prepare();

	virtual void cleanup();

	virtual void cleanupSwapChain();

	/// <summary>
	/// Create a swap chain that will be used in rendering images
	/// </summary>
	void createSwapChain();

	/// <summary>
	/// Create an image view object for use in the rendering pipeline
	/// 'Image View': describes how to access an image and what part of an image to access
	/// </summary>
	void createImageViews();

	vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectFlags);

	/// <summary>
	/// Create a rendering pass object which will tell vulkan information about framebuffer attachments:
	/// number of color and depth buffers, how many samples to use for each, how to handle contents
	/// </summary>
	void createRenderPass();

private:

};
}