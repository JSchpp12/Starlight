#include "StarEngine.hpp"

namespace star {


ConfigFile StarEngine::configFile = ConfigFile("./StarEngine.cfg"); 

ShaderManager StarEngine::shaderManager = ShaderManager(); 
TextureManager StarEngine::textureManager = TextureManager(StarEngine::configFile.GetSetting(Config_Settings::mediadirectory) + "images/texture.png");
LightManager StarEngine::lightManager = LightManager();
ObjectManager StarEngine::objectManager = ObjectManager();
MaterialManager StarEngine::materialManager = MaterialManager(std::make_unique<Material>(Material()));
MapManager StarEngine::mapManager = MapManager(std::unique_ptr<Texture>(new Texture(
	std::unique_ptr<std::vector<unsigned char>>(new std::vector<unsigned char>{ 0x00, 0x00, 0x00, 0x00 }),
	1, 1, 4)));
SceneBuilder StarEngine::sceneBuilder = SceneBuilder(StarEngine::objectManager, StarEngine::materialManager,
	StarEngine::textureManager, StarEngine::mapManager, StarEngine::lightManager);


void StarEngine::Run()
{
	while (!window->shouldClose()) {
		renderer->pollEvents();
		InteractionSystem::callWorldUpdates();
		renderer->draw();
	}
}

void StarEngine::init() {
	StarEngine::shaderManager.setDefault(StarEngine::configFile.GetSetting(Config_Settings::mediadirectory) + "shaders/default.vert",
		StarEngine::configFile.GetSetting(Config_Settings::mediadirectory) + "shaders/default.frag");
}

StarEngine::StarEngine(Camera& camera, std::vector<Handle> lightHandles, std::vector<Handle> objectHandles, RenderOptions& renderOptions) {
	this->init(); 

	//parse light information
	this->window = BasicWindow::New(800, 600, "Test");

	auto renderBuilder = BasicRenderer::Builder(*this->window, materialManager, textureManager, 
		mapManager, shaderManager, objectManager, camera, renderOptions);

	for (int i = 0; i < lightHandles.size(); i++) {
		renderBuilder.addLight(lightManager.resource(lightHandles[i])); 
	}

	for (int i = 0; i < objectHandles.size(); i++) {
		renderBuilder.addObject(objectManager.resource(objectHandles[i])); 
	}
	this->renderer = renderBuilder.build(); 
}
}