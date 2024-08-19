#include "StarEngine.hpp"

#define STB_IMAGE_IMPLEMENTATION   
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"

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
	RenderResourceSystem::init(*this->renderingDevice, mainRenderer->MAX_FRAMES_IN_FLIGHT);
	ManagerDescriptorPool descriptorManager(*this->renderingDevice);
	ManagerCommandBuffer commandBufferManager(*this->renderingDevice, mainRenderer->MAX_FRAMES_IN_FLIGHT);

	//objects will be prepared for render during the initialization of the main renderer
	mainRenderer->prepare();

	//prepare any shared resources
	StarObject::initSharedResources(*this->renderingDevice, this->mainRenderer->getMainExtent(), 
		this->mainRenderer->getMainRenderPass(), this->mainRenderer->MAX_FRAMES_IN_FLIGHT, 
		this->mainRenderer->getGlobalDescriptorLayout());

	while (!window->shouldClose()) {
		//check if any new objects have been added
		RenderResourceSystem::runInits(*this->renderingDevice, mainRenderer->MAX_FRAMES_IN_FLIGHT);

		int frameToDraw = this->mainRenderer->getFrameToBeDrawn(); 
		auto& objects = this->currentScene->getObjects();
		for (auto& obj : objects) {
			obj.second->prepDraw(frameToDraw); 
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