#include "StarEngine.hpp"

namespace star {

std::unique_ptr<std::string> StarEngine::screenshotPath = nullptr;

StarEngine::StarEngine() {
	ConfigFile::load("./StarEngine.cfg"); 

	this->window = BasicWindow::New(std::stoi(ConfigFile::getSetting(star::Config_Settings::resolution_x)), 
		std::stoi(ConfigFile::getSetting(star::Config_Settings::resolution_y)),
		ConfigFile::getSetting(star::Config_Settings::app_name));

	std::vector<star::Rendering_Features> features;
	this->renderingDevice = StarDevice::New(*window, features);

	int framesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));
	ManagerBuffer::init(*this->renderingDevice, framesInFlight);

	this->currentScene = std::unique_ptr<StarScene>(new StarScene(framesInFlight));
}

StarEngine::~StarEngine()
{
	this->renderingDevice->getDevice().waitIdle(); 
}

void StarEngine::Run()
{
	int framesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));

	ManagerDescriptorPool descriptorManager(*this->renderingDevice, framesInFlight);
	RenderResourceSystem::init(*this->renderingDevice, framesInFlight, mainRenderer->getMainExtent());
	ManagerCommandBuffer commandBufferManager(*this->renderingDevice, framesInFlight);

	//prepare any shared resources
	StarObject::initSharedResources(*this->renderingDevice, this->mainRenderer->getMainExtent(), 
		framesInFlight, 
		this->mainRenderer->getGlobalShaderInfo(), this->mainRenderer->getRenderingInfo());

	while (!window->shouldClose()) {
		//check if any new objects have been added
		RenderResourceSystem::runInits(*this->renderingDevice, framesInFlight, this->mainRenderer->getMainExtent());
		descriptorManager.update(framesInFlight); 
		ManagerBuffer::update(this->mainRenderer->getFrameToBeDrawn()); 

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
	this->mainRenderer = app.getMainRenderer(*this->renderingDevice, *this->window);
}
}