#include "StarEngine.hpp"

namespace star {

StarEngine::StarEngine() : currentScene(std::unique_ptr<StarScene>(new StarScene())) {
	ConfigFile::load("./StarEngine.cfg"); 
}

StarEngine::~StarEngine()
{
	this->renderingDevice->getDevice().waitIdle(); 
}

void StarEngine::Run()
{
	//objects will be prepared for render during the initialization of the main renderer

	mainRenderer->prepare();

	while (!window->shouldClose()) {
		int frameToDraw = this->mainRenderer->getFrameToBeDrawn(); 
		auto& objects = this->currentScene->getObjects();
		for (auto& obj : objects) {
			obj.second->prepDraw(frameToDraw); 
		}

		mainRenderer->pollEvents();
		InteractionSystem::callWorldUpdates();
		mainRenderer->submit();
	}
}

void StarEngine::init(StarApplication& app, RenderOptions& renderOptions) {
	//parse light information
	this->window = BasicWindow::New(800, 600, app.getApplicationName());

	this->renderingDevice = StarDevice::New(*window, app.getRequiredDeviceExtensions());

	this->mainRenderer = app.getMainRenderer(*this->renderingDevice, *this->window, renderOptions);

	this->mainRenderer->init(); 

	//prepare objects 
	auto& objects = this->currentScene->getObjects();

	for (auto& obj : objects) {
		obj.second->initRender(this->mainRenderer->MAX_FRAMES_IN_FLIGHT);
	}

	this->descriptorManager.build(*this->renderingDevice); 
}

}