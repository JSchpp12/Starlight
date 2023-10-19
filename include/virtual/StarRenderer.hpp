#pragma once 

#include "StarDevice.hpp"
#include "StarWindow.hpp"
#include "StarShader.hpp"
#include "FileResourceManager.hpp"
#include "StarCamera.hpp"
#include "ShaderManager.hpp"

#include <memory>

#include <vulkan/vulkan.hpp>

namespace star {
class StarRenderer {
public:
	virtual ~StarRenderer() = default; 
	StarRenderer(const StarRenderer&) = delete; 
	StarRenderer& operator=(const StarRenderer&) = delete; 

    void pollEvents();

    virtual void prepare(ShaderManager& shaderManager) = 0;

    virtual void draw() = 0; 

protected:
    struct RenderOptionsObject {
        bool drawMatAmbient;
    };

    struct GlobalUniformBufferObject {
        alignas(16) glm::mat4 proj;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 inverseView;              //used to extrapolate camera position, can be used to convert from camera to world space
        uint32_t numLights;                             //number of lights in render
        alignas(4) uint32_t renderOptions;
    };
    StarCamera& camera; 
    StarWindow& window;
    StarDevice& device;

	StarRenderer(StarWindow& window, StarCamera& inCamera, StarDevice& device) 
        : window(window), camera(inCamera), device(device){};

    //should be called in destructor
    //virtual void cleanupRenderer() = 0;

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