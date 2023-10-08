#pragma once

#include "BasicRenderer.hpp"
#include "BasicWindow.hpp"
#include "ConfigFile.hpp"
#include "Light.hpp"
#include "ShaderManager.hpp"
#include "LightManager.hpp"
#include "SceneBuilder.hpp"
#include "StarObject.hpp"
#include "StarScene.hpp"

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

	void init(RenderOptions& renderOptions, Camera& camera);


	std::vector<std::unique_ptr<StarObject>>& getObjList() { return this->objects; }
	StarScene& getScene() { return *this->currentScene; }
protected:
	std::unique_ptr<StarWindow> window;
	std::unique_ptr<StarDevice> renderingDevice; 
	std::unique_ptr<StarRenderer> renderer; 
	std::vector<std::unique_ptr<StarObject>> objects;
	std::vector<Handle> lightList; 
	std::unique_ptr<StarScene> currentScene; 
	LightManager lightManager;


private:

};
}