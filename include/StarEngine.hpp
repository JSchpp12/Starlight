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
#include "ManagerDescriptorPool.hpp"
#include "StarRenderGroup.hpp"
#include "RenderResourceSystem.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace star {
class StarEngine {
public:
	StarEngine();

	virtual ~StarEngine();

	void Run();

	void init(StarApplication& application, RenderOptions& renderOptions);

	StarScene& getScene() { return *this->currentScene; }
protected:
	std::unique_ptr<StarScene> currentScene;
	std::unique_ptr<StarWindow> window;
	std::unique_ptr<StarDevice> renderingDevice; 
	std::unique_ptr<SwapChainRenderer> mainRenderer; 
	std::vector<Handle> lightList; 

	LightManager lightManager;
	ManagerDescriptorPool descriptorManager;

	std::unique_ptr<StarBuffer> vertexBuffer, indexBuffer;
	std::vector<StarRenderGroup> renderGroups;  

};
}