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
	renderer->prepare(shaderManager); 

	while (!window->shouldClose()) {
		renderer->pollEvents();
		InteractionSystem::callWorldUpdates();
		renderer->draw();
	}
}

void StarEngine::init(StarApplication& app, RenderOptions& renderOptions) {
	StarEngine::shaderManager.setDefault(StarEngine::configFile.GetSetting(Config_Settings::mediadirectory) + "shaders/default.vert",
		StarEngine::configFile.GetSetting(Config_Settings::mediadirectory) + "shaders/default.frag");

	//parse light information
	this->window = BasicWindow::New(800, 600, app.getApplicationName());

	this->renderingDevice = StarDevice::New(*window);

	this->renderer = app.getRenderer(*renderingDevice, *window, renderOptions); 
}
}