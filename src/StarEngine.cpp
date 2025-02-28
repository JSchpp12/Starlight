#include "StarEngine.hpp"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace star {

std::unique_ptr<std::string> StarEngine::screenshotPath = nullptr;

StarEngine::StarEngine() {
	ConfigFile::load("./StarEngine.cfg"); 

	this->window = BasicWindow::New(std::stoi(ConfigFile::getSetting(star::Config_Settings::resolution_x)), 
		std::stoi(ConfigFile::getSetting(star::Config_Settings::resolution_y)),
		ConfigFile::getSetting(star::Config_Settings::app_name));

	std::set<star::Rendering_Features> features;
	{
		bool setting = false;
		std::istringstream(ConfigFile::getSetting(star::Config_Settings::required_device_feature_shader_float64)) >> std::boolalpha >> setting;

		if (setting) {
			features.insert(star::Rendering_Features::shader_float64);
		}
	}

	this->renderingDevice = StarDevice::New(*window, features);

	bool asyncTransfer = this->OVERRIDE_APPLY_SINGLE_THREAD_MODE || !this->renderingDevice->getHasDedicatedTransferQueue() ? false : true; 
	this->transferWorker = std::make_unique<TransferWorker>(*this->renderingDevice, this->renderingDevice->getAllocator(), this->renderingDevice->getTransferQueue(), this->renderingDevice->getCommandPool(star::Command_Buffer_Type::Ttransfer), asyncTransfer);

	int framesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));
	ManagerBuffer::init(*this->renderingDevice, *this->transferWorker, framesInFlight);

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

	uint8_t previousFrame = 0; 
	uint8_t currentFrame = 0; 
	while (!window->shouldClose()) {
		//check if any new objects have been added
		RenderResourceSystem::runInits(*this->renderingDevice, framesInFlight, this->mainRenderer->getMainExtent());
		descriptorManager.update(framesInFlight); 

		if (screenshotPath) {
			this->mainRenderer->triggerScreenshot(*screenshotPath);
			screenshotPath = nullptr;
		}

 		previousFrame = currentFrame; 
		currentFrame = mainRenderer->getFrameToBeDrawn();

		mainRenderer->pollEvents();
		InteractionSystem::callWorldUpdates(currentFrame);
		ManagerBuffer::update(currentFrame);
		vk::Semaphore allBuffersSubmitted = commandBufferManager.update(currentFrame);
		mainRenderer->submitPresentation(currentFrame, &allBuffersSubmitted);

		this->transferWorker->update();
	}

	this->renderingDevice->getDevice().waitIdle();
	ManagerBuffer::cleanup(*this->renderingDevice);
	RenderResourceSystem::cleanup(*this->renderingDevice);
	StarObject::cleanupSharedResources(*this->renderingDevice); 
}

void StarEngine::init(StarApplication& app) {
	this->mainRenderer = app.getMainRenderer(*this->renderingDevice, *this->window);
}
}