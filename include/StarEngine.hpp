#pragma once

#include "BasicRenderer.hpp"
#include "BasicWindow.hpp"
#include "ConfigFile.hpp"
#include "Light.hpp"
#include "StarApplication.hpp "
#include "ShaderManager.hpp"
#include "LightManager.hpp"
#include "SceneBuilder.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace star {
class StarEngine {
public:
	StarEngine();

	static ConfigFile configFile;
	static ShaderManager shaderManager;

	static MapManager mapManager;


	static std::string GetSetting(Config_Settings setting) {
		return configFile.GetSetting(setting);
	}

	virtual ~StarEngine() = default;

	void Run();

	void init(RenderOptions& renderOptions, StarApplication& app);


	std::vector<Handle>& getObjList() { return this->objList; }
	std::vector<Handle>& getLightList() { return this->lightList; }
	ObjectManager& getObjectManager() { return this->objectManager; }
	LightManager& getLightManager() { return this->lightManager; }
	SceneBuilder& getSceneBuilder() { return *this->sceneBuilder; }
protected:
	std::unique_ptr<StarWindow> window;
	std::unique_ptr<StarDevice> renderingDevice; 
	std::unique_ptr<StarRenderer> renderer; 
	std::vector<Handle> objList; 
	std::vector<Handle> lightList; 

	LightManager lightManager;
	ObjectManager objectManager;
	std::unique_ptr<SceneBuilder> sceneBuilder;

private:

};
}