#pragma once

#include "ManagerBuffer.hpp"
#include "SwapChainRenderer.hpp"
#include "BasicWindow.hpp"
#include "Color.hpp"
#include "ConfigFile.hpp"
#include "Light.hpp"
#include "StarObject.hpp"
#include "StarScene.hpp"
#include "StarApplication.hpp"
#include "StarCommandBuffer.hpp"
#include "ManagerDescriptorPool.hpp"
#include "StarRenderGroup.hpp"
#include "ManagerCommandBuffer.hpp"
#include "TransferWorker.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace star {
class StarEngine {
public:
	static void takeScreenshot(const std::string& path) { 
		screenshotPath = std::make_unique<std::string>(path); 
	}

	StarEngine();

	virtual ~StarEngine();

	void Run();

	void init(StarApplication& application);

	StarScene& getScene() { return *this->currentScene; }
protected:
	std::unique_ptr<StarWindow> window;
	std::unique_ptr<StarDevice> renderingDevice;
	std::unique_ptr<StarScene> currentScene;
	std::unique_ptr<SwapChainRenderer> mainRenderer; 
	std::unique_ptr<TransferWorker> transferWorker;

	std::vector<Handle> lightList; 

	std::unique_ptr<StarBuffer> vertexBuffer, indexBuffer;
	std::vector<StarRenderGroup> renderGroups;  

private: 
	static std::unique_ptr<std::string> screenshotPath;

	const bool OVERRIDE_APPLY_SINGLE_THREAD_MODE = true;
};
}