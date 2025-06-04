#include "StarEngine.hpp"
#include "ConfigFile.hpp"
#include "Enums.hpp"
#include "ManagerRenderResource.hpp"
#include "SwapChainRenderer.hpp"
#include "ManagerCommandBuffer.hpp"
#include "ManagerDescriptorPool.hpp"
#include "StarCommandBuffer.hpp"
#include "StarRenderGroup.hpp"
#include "RenderResourceSystem.hpp"

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <stdexcept>

namespace star
{
StarEngine::StarEngine(std::unique_ptr<StarApplication> nApplication)
    : application(std::move(nApplication)), window(CreateStarWindow()), renderingDevice(CreateStarDevice(*this->window))
{
    this->transferWorker =
        std::make_unique<TransferWorker>(*this->renderingDevice, this->OVERRIDE_APPLY_SINGLE_THREAD_MODE);

    uint8_t framesInFlight; 
    {
        int readFramesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));
        if (!CastHelpers::SafeCast<int, uint8_t>(readFramesInFlight, framesInFlight)){
            throw std::runtime_error("Invalid number of frames in flight in config file"); 
        }
    }

    StarManager::init(*this->renderingDevice, *this->transferWorker);

    this->application->init(*this->renderingDevice, *this->window, framesInFlight);

    this->mainRenderer = application->getPresentationRenderer();
}

StarEngine::~StarEngine()
{
    this->renderingDevice->getDevice().waitIdle();
}

void StarEngine::run()
{
    int framesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));

    ManagerDescriptorPool descriptorManager(*this->renderingDevice, framesInFlight);
    RenderResourceSystem::init(*this->renderingDevice, framesInFlight, this->window->getExtent());
    ManagerCommandBuffer commandBufferManager(*this->renderingDevice, framesInFlight);

    // prepare any shared resources
    StarObject::initSharedResources(*this->renderingDevice, this->window->getExtent(), framesInFlight,
                                    this->mainRenderer->getGlobalShaderInfo(), this->mainRenderer->getRenderingInfo());

    uint8_t currentFrame = 0;
    while (!window->shouldClose())
    {
        currentFrame = this->mainRenderer->getFrameToBeDrawn();

        // check if any new objects have been added
        RenderResourceSystem::runInits(*this->renderingDevice, framesInFlight, this->window->getExtent());
        descriptorManager.update(framesInFlight);

        this->mainRenderer->pollEvents();
        InteractionSystem::callWorldUpdates(currentFrame);
        ManagerRenderResource::update(currentFrame);
        vk::Semaphore allBuffersSubmitted = commandBufferManager.update(currentFrame);
        this->mainRenderer->submitPresentation(currentFrame, &allBuffersSubmitted);
        this->transferWorker->update();
    }

    this->renderingDevice->getDevice().waitIdle();
    ManagerRenderResource::cleanup(*this->renderingDevice);
    RenderResourceSystem::cleanup(*this->renderingDevice);
    StarObject::cleanupSharedResources(*this->renderingDevice);
}

std::unique_ptr<star::StarWindow> star::StarEngine::CreateStarWindow()
{
    return StarWindow::Builder()
        .setWidth(std::stoi(ConfigFile::getSetting(star::Config_Settings::resolution_x)))
        .setHeight(std::stoi(ConfigFile::getSetting(star::Config_Settings::resolution_y)))
        .setTitle(ConfigFile::getSetting(star::Config_Settings::app_name))
        .build();
}

std::unique_ptr<star::StarDevice> star::StarEngine::CreateStarDevice(StarWindow &window)
{
    std::set<star::Rendering_Features> features;
    {
        bool setting = false;
        std::istringstream(ConfigFile::getSetting(star::Config_Settings::required_device_feature_shader_float64)) >>
            std::boolalpha >> setting;

        if (setting)
        {
            features.insert(star::Rendering_Features::shader_float64);
        }
    }

    return StarDevice::New(window, features);
}
} // namespace star