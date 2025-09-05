#include "StarEngine.hpp"
#include "ConfigFile.hpp"
#include "Enums.hpp"
#include "ManagerDescriptorPool.hpp"
#include "RenderResourceSystem.hpp"
#include "RenderingInstance.hpp"
#include "StarCommandBuffer.hpp"
#include "StarRenderGroup.hpp"
#include "job/TaskManager.hpp"
#include "renderer/SwapChainRenderer.hpp"


#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <stdexcept>
#include <vk_mem_alloc.h>

#include "job/Worker.hpp"
#include "job/tasks/TaskFactory.hpp"


namespace star
{
StarEngine::StarEngine(std::unique_ptr<StarApplication> nApplication)
    : application(std::move(nApplication)), window(CreateStarWindow()),
      deviceManager(core::RenderingInstance(ConfigFile::getSetting(star::Config_Settings::app_name)))
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

    std::set<Rendering_Device_Features> renderingFeatures{Rendering_Device_Features::timeline_semaphores};

    uint8_t framesInFlight;
    {
        int readFramesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));
        if (!CastHelpers::SafeCast<int, uint8_t>(readFramesInFlight, framesInFlight))
        {
            throw std::runtime_error("Invalid number of frames in flight in config file");
        }
    }

    deviceManager.createDevice(core::device::DeviceID{.id = uint8_t(0)}, frameCounter, framesInFlight, features,
                               *this->window, renderingFeatures);

    // try and get a transfer queue from different queue fams
    {
        std::set<uint32_t> selectedFamilyIndices = std::set<uint32_t>();
        std::vector<StarQueue> transferWorkerQueues = std::vector<StarQueue>();

        const auto transferFams =
            this->deviceManager.getContext().getDevice().getQueueOwnershipTracker().getQueueFamiliesWhichSupport(
                vk::QueueFlagBits::eTransfer);

        for (const auto &fam : transferFams)
        {
            if (selectedFamilyIndices.size() > 1)
                break;

            if (fam != deviceManager.getContext()
                           .getDevice()
                           .getDefaultQueue(Queue_Type::Tgraphics)
                           .getParentQueueFamilyIndex() &&
                fam != deviceManager.getContext()
                           .getDevice()
                           .getDefaultQueue(Queue_Type::Tcompute)
                           .getParentQueueFamilyIndex() &&
                !selectedFamilyIndices.contains(fam))
            {
                auto nQueue =
                    deviceManager.getContext().getDevice().getQueueOwnershipTracker().giveMeQueueWithProperties(
                        vk::QueueFlagBits::eTransfer, false, fam);

                if (nQueue.has_value())
                {
                    transferWorkerQueues.push_back(nQueue.value());

                    selectedFamilyIndices.insert(fam);
                }
            }
        }
    }

    this->application->init();
}

StarEngine::~StarEngine()
{
    RenderResourceSystem::cleanup(deviceManager.getContext());
    StarObject::cleanupSharedResources(deviceManager.getContext());
}

void StarEngine::run()
{
    const uint8_t numFramesInFlight = GetNumFramesInFlight();

    std::shared_ptr<StarScene> currentScene =
        this->application->loadScene(deviceManager.getContext(), *this->window, numFramesInFlight);

    core::device::managers::ManagerDescriptorPool descriptorManager(deviceManager.getContext(), numFramesInFlight);
    RenderResourceSystem::init(deviceManager.getContext(), numFramesInFlight, this->window->getExtent());

    // prepare any shared resources
    StarObject::initSharedResources(deviceManager.getContext(), this->window->getExtent(), numFramesInFlight,
                                    currentScene->getPresentationRenderer()->getGlobalShaderInfo(),
                                    currentScene->getPresentationRenderer()->getRenderTargetInfo());

    uint8_t frameInFlightIndex = 0;
    while (!window->shouldClose())
    {
        frameInFlightIndex = currentScene->getPresentationRenderer()->getFrameToBeDrawn();

        // check if any new objects have been added
        deviceManager.getContext().prepareForNextFrame(); 

        RenderResourceSystem::runInits(deviceManager.getContext(), numFramesInFlight, this->window->getExtent());
        currentScene->frameUpdate(deviceManager.getContext()); 
        descriptorManager.update(numFramesInFlight);

        currentScene->getPresentationRenderer()->pollEvents();
        InteractionSystem::callWorldUpdates(frameInFlightIndex);
        ManagerRenderResource::update(deviceManager.getContext().getDeviceID(), frameInFlightIndex);
        vk::Semaphore allBuffersSubmitted =
            deviceManager.getContext().getManagerCommandBuffer().update(frameInFlightIndex);
        currentScene->getPresentationRenderer()->submitPresentation(frameInFlightIndex, &allBuffersSubmitted);
        this->deviceManager.getContext().getTransferWorker().update();

        frameCounter++;
    }

    deviceManager.getContext().getDevice().getVulkanDevice().waitIdle();
    ManagerRenderResource::cleanup(star::core::device::DeviceID(0), deviceManager.getContext().getDevice());
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
        if (!star::CastHelpers::SafeCast<int, uint8_t>(framesInFlight, num)){
            throw std::runtime_error("Faled to process number of frames in flight"); 
        } 
    }

    return num; 
}
} // namespace star