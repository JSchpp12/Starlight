#include "StarEngine.hpp"
#include "ConfigFile.hpp"
#include "Enums.hpp"
#include "ManagerDescriptorPool.hpp"
#include "RenderResourceSystem.hpp"
#include "RenderingInstance.hpp"
#include "StarCommandBuffer.hpp"
#include "StarRenderGroup.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "job/TaskManager.hpp"
#include "renderer/SwapChainRenderer.hpp"

#include "service/ScreenCapture.hpp"

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <stdexcept>

namespace star
{
StarEngine::StarEngine(StarApplication &application)
    : m_application(application), window(CreateStarWindow()),
      deviceManager(core::RenderingInstance(ConfigFile::getSetting(star::Config_Settings::app_name)))
{
    core::logging::init();
    core::logging::log(boost::log::trivial::info, "Logger initialized");

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

    std::set<Rendering_Device_Features> renderingFeatures{Rendering_Device_Features::timeline_semaphores};

    uint8_t framesInFlight;
    {
        int readFramesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));
        if (!CastHelpers::SafeCast<int, uint8_t>(readFramesInFlight, framesInFlight))
        {
            throw std::runtime_error("Invalid number of frames in flight in config file");
        }
    }
    deviceManager.createDevice(defaultDevice, frameCounter, framesInFlight, features, *this->window, renderingFeatures);

    // try and get a transfer queue from different queue fams
    {
        std::set<uint32_t> selectedFamilyIndices = std::set<uint32_t>();
        std::vector<StarQueue> transferWorkerQueues = std::vector<StarQueue>();

        const auto transferFams = this->deviceManager.getContext(defaultDevice)
                                      .getDevice()
                                      .getQueueOwnershipTracker()
                                      .getQueueFamiliesWhichSupport(vk::QueueFlagBits::eTransfer);

        for (const auto &fam : transferFams)
        {
            if (selectedFamilyIndices.size() > 1)
                break;

            if (fam != deviceManager.getContext(defaultDevice)
                           .getDevice()
                           .getDefaultQueue(Queue_Type::Tgraphics)
                           .getParentQueueFamilyIndex() &&
                fam != deviceManager.getContext(defaultDevice)
                           .getDevice()
                           .getDefaultQueue(Queue_Type::Tcompute)
                           .getParentQueueFamilyIndex() &&
                !selectedFamilyIndices.contains(fam))
            {
                auto nQueue = deviceManager.getContext(defaultDevice)
                                  .getDevice()
                                  .getQueueOwnershipTracker()
                                  .giveMeQueueWithProperties(vk::QueueFlagBits::eTransfer, false, fam);

                if (nQueue.has_value())
                {
                    transferWorkerQueues.push_back(nQueue.value());

                    selectedFamilyIndices.insert(fam);
                }
            }
        }
    }

    m_application.init();
}

StarEngine::~StarEngine()
{
    RenderResourceSystem::cleanup(deviceManager.getContext(defaultDevice));
    StarObject::cleanupSharedResources(deviceManager.getContext(defaultDevice));
}

void StarEngine::run()
{
    const uint8_t numFramesInFlight = GetNumFramesInFlight();

    std::shared_ptr<StarScene> currentScene =
        m_application.loadScene(deviceManager.getContext(defaultDevice), *this->window, numFramesInFlight);

    assert(currentScene && "Application must provide a proper instance of a scene object");

    currentScene->prepRender(deviceManager.getContext(defaultDevice), numFramesInFlight);

    core::device::managers::ManagerDescriptorPool descriptorManager(deviceManager.getContext(defaultDevice),
                                                                    numFramesInFlight);
    RenderResourceSystem::init(deviceManager.getContext(defaultDevice), numFramesInFlight, this->window->getExtent());

    // prepare any shared resources
    StarObject::initSharedResources(deviceManager.getContext(defaultDevice), this->window->getExtent(),
                                    numFramesInFlight, currentScene->getPresentationRenderer()->getGlobalShaderInfo(),
                                    currentScene->getPresentationRenderer()->getRenderTargetInfo());

    initServices(*currentScene->getPresentationRenderer());

    uint8_t frameInFlightIndex = 0;
    while (!window->shouldClose())
    {
        frameInFlightIndex = currentScene->getPresentationRenderer()->getFrameToBeDrawn();

        // check if any new objects have been added
        deviceManager.getContext(defaultDevice).prepareForNextFrame(frameInFlightIndex);

        RenderResourceSystem::runInits(deviceManager.getContext(defaultDevice), numFramesInFlight,
                                       this->window->getExtent());
        descriptorManager.update(numFramesInFlight);

        currentScene->getPresentationRenderer()->pollEvents();
        InteractionSystem::callWorldUpdates(frameInFlightIndex);

        currentScene->frameUpdate(deviceManager.getContext(defaultDevice), frameInFlightIndex);

        ManagerRenderResource::frameUpdate(deviceManager.getContext(defaultDevice).getDeviceID(), frameInFlightIndex);
        vk::Semaphore allBuffersSubmitted =
            deviceManager.getContext(defaultDevice)
                .getManagerCommandBuffer()
                .update(frameInFlightIndex, deviceManager.getContext(defaultDevice).getCurrentFrameIndex());
        currentScene->getPresentationRenderer()->submitPresentation(frameInFlightIndex, &allBuffersSubmitted);
        this->deviceManager.getContext(defaultDevice).getTransferWorker().update();
    }

    deviceManager.getContext(defaultDevice).waitIdle();
    currentScene->cleanupRender(deviceManager.getContext(defaultDevice));
}

std::unique_ptr<star::StarWindow> star::StarEngine::CreateStarWindow()
{
    return StarWindow::Builder()
        .setWidth(std::stoi(ConfigFile::getSetting(star::Config_Settings::resolution_x)))
        .setHeight(std::stoi(ConfigFile::getSetting(star::Config_Settings::resolution_y)))
        .setTitle(ConfigFile::getSetting(star::Config_Settings::app_name))
        .build();
}

std::unique_ptr<job::TaskManager> StarEngine::CreateManager()
{
    std::unique_ptr<job::TaskManager> mgr = std::make_unique<job::TaskManager>();

    return mgr;
}

uint8_t StarEngine::GetNumFramesInFlight()
{
    uint8_t num;
    {
        int framesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));
        if (!star::CastHelpers::SafeCast<int, uint8_t>(framesInFlight, num))
        {
            throw std::runtime_error("Faled to process number of frames in flight");
        }
    }

    return num;
}

void star::StarEngine::initServices(star::core::renderer::SwapChainRenderer &presentationRenderer)
{
    deviceManager.getContext(defaultDevice)
        .registerService(star::service::Service{star::service::ScreenCapture{
            presentationRenderer.getRenderToColorImages(), presentationRenderer.getDoneSemaphores()}});
}

} // namespace star