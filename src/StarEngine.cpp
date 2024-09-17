#include "StarEngine.hpp"

namespace star {

std::unique_ptr<std::string> StarEngine::screenshotPath = nullptr;

StarEngine::StarEngine() : currentScene(std::unique_ptr<StarScene>(new StarScene())) {
	ConfigFile::load("./StarEngine.cfg"); 
}

StarEngine::~StarEngine()
{
	this->renderingDevice->getDevice().waitIdle(); 
}

void StarEngine::Run()
{
	ManagerDescriptorPool descriptorManager(*this->renderingDevice, mainRenderer->MAX_FRAMES_IN_FLIGHT);
	RenderResourceSystem::init(*this->renderingDevice, mainRenderer->MAX_FRAMES_IN_FLIGHT, mainRenderer->getMainExtent());
	ManagerCommandBuffer commandBufferManager(*this->renderingDevice, mainRenderer->MAX_FRAMES_IN_FLIGHT);

	//prepare any shared resources
	StarObject::initSharedResources(*this->renderingDevice, this->mainRenderer->getMainExtent(), 
		this->mainRenderer->MAX_FRAMES_IN_FLIGHT, 
		this->mainRenderer->getGlobalDescriptorLayout(), this->mainRenderer->getRenderingInfo());

	while (!window->shouldClose()) {
		//check if any new objects have been added
		RenderResourceSystem::runInits(*this->renderingDevice, this->mainRenderer->MAX_FRAMES_IN_FLIGHT, this->mainRenderer->getMainExtent());

		int frameToDraw = this->mainRenderer->getFrameToBeDrawn(); 
		for (StarObject& obj : this->currentScene->getObjects()) {
			obj.prepDraw(frameToDraw);
		}

		if (screenshotPath) {
			this->mainRenderer->triggerScreenshot(*screenshotPath);
			screenshotPath = nullptr;
		}

		mainRenderer->pollEvents();
		InteractionSystem::callWorldUpdates();
		const uint32_t frameIndex = mainRenderer->getFrameToBeDrawn();
		vk::Semaphore allBuffersSubmitted = commandBufferManager.update(frameIndex);
		mainRenderer->submitPresentation(frameIndex, &allBuffersSubmitted);
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