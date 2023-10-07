#include "StarEngine.hpp"
#include "StarEngine.hpp"

namespace star {


ConfigFile StarEngine::configFile = ConfigFile("./StarEngine.cfg"); 

ShaderManager StarEngine::shaderManager = ShaderManager(); 
//TextureManager StarEngine::textureManager = TextureManager(StarEngine::configFile.GetSetting(Config_Settings::mediadirectory) + "images/texture.png");
MapManager StarEngine::mapManager = MapManager(std::unique_ptr<Texture>(new Texture(
	std::unique_ptr<std::vector<unsigned char>>(new std::vector<unsigned char>{ 0x00, 0x00, 0x00, 0x00 }),
	1, 1, 4)));

StarEngine::StarEngine() {
	sceneBuilder = std::unique_ptr<SceneBuilder>(new SceneBuilder(objectManager, mapManager, lightManager)); 
}

void StarEngine::Run()
{
	while (!window->shouldClose()) {
		renderer->pollEvents();
		InteractionSystem::callWorldUpdates();
		renderer->draw();
	}
}

void StarEngine::init(RenderOptions& renderOptions, StarApplication& app) {
	StarEngine::shaderManager.setDefault(StarEngine::configFile.GetSetting(Config_Settings::mediadirectory) + "shaders/default.vert",
		StarEngine::configFile.GetSetting(Config_Settings::mediadirectory) + "shaders/default.frag");

	//parse light information
	this->window = BasicWindow::New(800, 600, "Test");

	this->renderingDevice = StarDevice::New(*window);
	auto renderBuilder = BasicRenderer::Builder(*this->window,
		mapManager, shaderManager, objectManager, app.getCamera(), renderOptions, *this->renderingDevice);

	for (int i = 0; i < lightList.size(); i++) {
		renderBuilder.addLight(lightManager.resource(lightList[i]));
	}

	for (int i = 0; i < objList.size(); i++) {
		renderBuilder.addObject(objectManager.resource(objList[i]));
	}
	this->renderer = renderBuilder.build();
}
}