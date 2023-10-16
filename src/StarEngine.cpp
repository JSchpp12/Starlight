#include "StarEngine.hpp"

namespace star {


ConfigFile StarEngine::configFile = ConfigFile("./StarEngine.cfg"); 

ShaderManager StarEngine::shaderManager = ShaderManager(); 
//TextureManager StarEngine::textureManager = TextureManager(StarEngine::configFile.GetSetting(Config_Settings::mediadirectory) + "images/texture.png");
MapManager StarEngine::mapManager = MapManager(std::make_unique<Texture>(1, 1));

StarEngine::StarEngine() : currentScene(std::unique_ptr<StarScene>(new StarScene())) {
	//sceneBuilder = std::unique_ptr<SceneBuilder>(new SceneBuilder(objectManager, mapManager, lightManager)); 
}

void StarEngine::Run()
{
	while (!window->shouldClose()) {
		renderer->pollEvents();
		InteractionSystem::callWorldUpdates();
		renderer->draw();
	}
}

void StarEngine::init(RenderOptions& renderOptions, StarCamera& camera) {
	StarEngine::shaderManager.setDefault(StarEngine::configFile.GetSetting(Config_Settings::mediadirectory) + "shaders/default.vert",
		StarEngine::configFile.GetSetting(Config_Settings::mediadirectory) + "shaders/default.frag");

	//parse light information
	this->window = BasicWindow::New(1600, 1200, "Test");

	this->renderingDevice = StarDevice::New(*window);
	auto renderBuilder = BasicRenderer::Builder(*this->window,
		mapManager, shaderManager, camera, renderOptions, *this->renderingDevice);
	for (auto& light : this->currentScene->getLights()) {
		renderBuilder.addLight(*light);
	}
	for (auto& obj : this->currentScene->getObjects()) {
		renderBuilder.addObject(*obj);
	}

	this->renderer = renderBuilder.build();
}
}