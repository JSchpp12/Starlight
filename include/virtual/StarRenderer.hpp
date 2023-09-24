#pragma once 

#include "StarDevice.hpp"
#include "StarWindow.hpp"
#include "StarShader.hpp"
#include "FileResourceManager.hpp"
#include "GameObject.hpp"
#include "Camera.hpp"

#include <memory>

#include <vulkan/vulkan.hpp>

namespace star {
class StarRenderer {
public:
	virtual ~StarRenderer() = default; 
	StarRenderer(const StarRenderer&) = delete; 
	StarRenderer& operator=(const StarRenderer&) = delete; 

    void pollEvents();

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
    FileResourceManager<GameObject>& objectManager; 
    FileResourceManager<StarShader>& shaderManager;
    Camera& camera;
    StarWindow& window;

    std::unique_ptr<StarDevice> device;

	StarRenderer(StarWindow& window, FileResourceManager<StarShader>& shaderManager, 
        FileResourceManager<GameObject>& objectManager, Camera& inCamera) 
        : window(window), shaderManager(shaderManager), objectManager(objectManager), camera(inCamera){};

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