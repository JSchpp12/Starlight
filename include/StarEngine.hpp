#pragma once

#include "BasicRenderer.hpp"
#include "BasicWindow.hpp"
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
		Builder(StarApplication<ShaderManager, TextureManager, LightManager, SceneBuilder>& application) : application(application) {};
		std::unique_ptr<StarEngine> build() {
			auto lightHandles = application.getLights();
			auto lightManager = application.getLightManager(); 
			return std::unique_ptr<StarEngine>(new StarEngine(lightHandles, lightManager));
		}

	private:
		StarApplication< ShaderManager, TextureManager, LightManager, SceneBuilder>& application;

	};

	virtual ~StarEngine() = default;

protected:
	std::unique_ptr<StarWindow> window;
	std::unique_ptr<StarRenderer> renderer; 
	std::vector<Handle> lightHandles;

	StarEngine(std::vector<Handle> lightHandles, LightManager* lightManager);

private:

};
}