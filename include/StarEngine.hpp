#pragma once

#include "SwapChainRenderer.hpp"
#include "BasicWindow.hpp"
#include "Color.hpp"
#include "ConfigFile.hpp"
#include "Light.hpp"
#include "ShaderManager.hpp"
#include "LightManager.hpp"
#include "StarObject.hpp"
#include "StarScene.hpp"
#include "StarApplication.hpp"
#include "StarCommandBuffer.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace star {
class StarEngine {
public:
	StarEngine();

	virtual ~StarEngine() = default;

	void Run();

	void init(StarApplication& application, RenderOptions& renderOptions);

	std::vector<std::unique_ptr<StarObject>>& getObjList() { return this->objects; }
	StarScene& getScene() { return *this->currentScene; }
protected:
	std::vector<std::unique_ptr<StarObject>> objects;
	std::unique_ptr<StarScene> currentScene;
	std::unique_ptr<StarWindow> window;
	std::unique_ptr<StarDevice> renderingDevice; 
	std::unique_ptr<SwapChainRenderer> mainRenderer; 
	std::vector<Handle> lightList; 

	std::vector<std::reference_wrapper<StarCommandBuffer>> registeredCommandBuffers; 

	LightManager lightManager;

};
}