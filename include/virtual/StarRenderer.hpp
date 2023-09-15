#pragma once 

#include "StarDevice.hpp"
#include "StarWindow.hpp"

#include <memory>

#include <vulkan/vulkan.hpp>

namespace star {
class StarRenderer {
public:
	virtual ~StarRenderer() = default; 
	StarRenderer(const StarRenderer&) = delete; 
	StarRenderer& operator=(const StarRenderer&) = delete; 

protected:
	std::unique_ptr<StarDevice> device{};
	StarWindow& window; 

	StarRenderer(StarWindow& window) : window(window) {};

#pragma region helpers
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

	//Look through givent present modes and pick the "best" one
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

	/// <summary>
	/// Helper function -- TODO 
	/// </summary>
	/// <returns></returns>
	vk::Format findDepthFormat();
#pragma endregion


private: 

};
}