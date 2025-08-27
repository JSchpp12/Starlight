#include "StarEngine.hpp"
#include "ConfigFile.hpp"
#include "Enums.hpp"
#include "ManagerDescriptorPool.hpp"
#include "RenderResourceSystem.hpp"
#include "RenderingInstance.hpp"
#include "StarCommandBuffer.hpp"
#include "StarRenderGroup.hpp"
#include "SwapChainRenderer.hpp"
#include "job/TaskManager.hpp"

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <stdexcept>
#include <vk_mem_alloc.h>

#include "job/tasks/TaskFactory.hpp"
#include "job/Worker.hpp"

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

    uint8_t framesInFlight;
    {
        int readFramesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));
        if (!CastHelpers::SafeCast<int, uint8_t>(readFramesInFlight, framesInFlight))
        {
            throw std::runtime_error("Invalid number of frames in flight in config file");
        }
    }

    deviceManager.createDevice(frameCounter, framesInFlight, features, *this->window);

    // try and get a transfer queue from different queue fams
    {
        std::set<uint32_t> selectedFamilyIndices = std::set<uint32_t>();
        std::vector<StarQueue> transferWorkerQueues = std::vector<StarQueue>();

        const auto transferFams =
            this->deviceManager.getContext().getDevice().getQueueOwnershipTracker().getQueueFamiliesWhichSupport(
                vk::QueueFlagBits::eTransfer);
        for (const auto &fam : transferFams)
        {
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

    this->application->init(deviceManager.getContext(), *this->window, framesInFlight);

    this->mainRenderer = application->getPresentationRenderer();
}

StarEngine::~StarEngine()
{
    RenderResourceSystem::cleanup(deviceManager.getContext());
    StarObject::cleanupSharedResources(deviceManager.getContext());
}

void StarEngine::run()
{
    int framesInFlight = std::stoi(ConfigFile::getSetting(Config_Settings::frames_in_flight));

    core::device::managers::ManagerDescriptorPool descriptorManager(deviceManager.getContext(), framesInFlight);
    RenderResourceSystem::init(deviceManager.getContext(), framesInFlight, this->window->getExtent());

    // prepare any shared resources
    StarObject::initSharedResources(deviceManager.getContext(), this->window->getExtent(), framesInFlight,
                                    this->mainRenderer->getGlobalShaderInfo(),
                                    this->mainRenderer->getRenderTargetInfo());

    uint8_t frameInFlightIndex = 0;
    while (!window->shouldClose())
    {
        frameInFlightIndex = this->mainRenderer->getFrameToBeDrawn();

        // check if any new objects have been added
        RenderResourceSystem::runInits(deviceManager.getContext(), framesInFlight, this->window->getExtent());
        descriptorManager.update(framesInFlight);

        this->mainRenderer->pollEvents();
        InteractionSystem::callWorldUpdates(frameInFlightIndex);
        ManagerRenderResource::update(frameInFlightIndex); 
        // ManagerRenderResource::update(frameInFlightIndex);
        vk::Semaphore allBuffersSubmitted = deviceManager.getContext().getManagerCommandBuffer().update(frameInFlightIndex);
        this->mainRenderer->submitPresentation(frameInFlightIndex, &allBuffersSubmitted);
        this->deviceManager.getContext().getTransferWorker().update();

        frameCounter++; 
    }

    deviceManager.getContext().getDevice().getVulkanDevice().waitIdle();
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
} // namespace star