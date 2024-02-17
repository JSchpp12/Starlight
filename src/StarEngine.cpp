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
	RenderResourceSystem::preparePrimaryGeometry(*this->renderingDevice);
	RenderResourceSystem::runInits(*this->renderingDevice, mainRenderer->MAX_FRAMES_IN_FLIGHT);

	this->descriptorManager.build(*this->renderingDevice);

	//objects will be prepared for render during the initialization of the main renderer
	mainRenderer->prepare();

	//prepare any shared resources
	StarObject::initSharedResources(*this->renderingDevice, this->mainRenderer->getMainExtent(), 
		this->mainRenderer->getMainRenderPass(), this->mainRenderer->MAX_FRAMES_IN_FLIGHT, 
		this->mainRenderer->getGlobalDescriptorLayout());

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

	this->renderingDevice->getDevice().waitIdle();
	RenderResourceSystem::cleanup(*this->renderingDevice);
	StarObject::cleanupSharedResources(*this->renderingDevice); 
}

void StarEngine::init(StarApplication& app) {
	this->window = BasicWindow::New(app.getCamera().getResolution().x, app.getCamera().getResolution().y, app.getApplicationName());

	this->renderingDevice = StarDevice::New(*window, app.getRequiredDeviceExtensions());

	this->mainRenderer = app.getMainRenderer(*this->renderingDevice, *this->window);
}
}