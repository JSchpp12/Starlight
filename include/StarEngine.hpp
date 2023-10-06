#pragma once

#include "BasicRenderer.hpp"
#include "BasicWindow.hpp"
#include "ConfigFile.hpp"
#include "Light.hpp"
#include "StarApplication.hpp "
#include "ShaderManager.hpp"
#include "TextureManager.hpp"
#include "LightManager.hpp"
#include "SceneBuilder.hpp"

#include <memory>
#include <vector>

namespace star {
class StarEngine {
public:
	class Builder {
	public:
		Builder(){};

		std::unique_ptr<StarEngine> build(Camera& camera, std::vector<Handle> lightHandles, std::vector<Handle> objectHandles, RenderOptions& renderOptions) {
			auto engine = std::unique_ptr<StarEngine>(new StarEngine(camera, lightHandles, objectHandles, renderOptions));
			return std::move(engine); 
		}

	private:
	};

	static ConfigFile configFile;
	static ShaderManager shaderManager;
	static TextureManager textureManager;
	static LightManager lightManager;
	static ObjectManager objectManager;
	static MapManager mapManager;
	static SceneBuilder sceneBuilder;

	static std::string GetSetting(Config_Settings setting) {
		return configFile.GetSetting(setting);
	}

	virtual ~StarEngine() = default;

	void Run();

	void init(); 

protected:
	std::unique_ptr<StarWindow> window;
	std::unique_ptr<StarRenderer> renderer; 

	StarEngine(Camera& camera, std::vector<Handle> lightHandles, std::vector<Handle> objectHandles, RenderOptions& renderOptions);

private:

};
}