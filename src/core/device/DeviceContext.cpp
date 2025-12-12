#include "core/device/DeviceContext.hpp"

#include "core/logging/LoggingFactory.hpp"
#include "job/tasks/TaskFactory.hpp"
#include "job/worker/DefaultWorker.hpp"
#include "job/worker/Worker.hpp"
#include "job/worker/default_worker/detail/ThreadTaskHandlingPolicies.hpp"

#include "core/device/system/event/StartOfNextFrame.hpp"
#include "managers/ManagerRenderResource.hpp"

#include <starlight/common/HandleTypeRegistry.hpp>
#include <starlight/common/helper/CastHelpers.hpp>

#include "service/InitParameters.hpp"

#include <cassert>

star::core::device::DeviceContext::DeviceContext(DeviceContext &&other)
    : m_frameInFlightTrackingInfo(std::move(other.m_frameInFlightTrackingInfo)),
      m_deviceID(std::move(other.m_deviceID)), m_surface(std::move(other.m_surface)),
      m_device(std::move(other.m_device)), m_eventBus(std::move(other.m_eventBus)),
      m_taskManager(std::move(other.m_taskManager)), m_graphicsManagers(std::move(other.m_graphicsManagers)),
      m_commandBufferManager(std::move(other.m_commandBufferManager)),
      m_transferWorker(std::move(other.m_transferWorker)),
      m_renderResourceManager(std::move(other.m_renderResourceManager)), m_services(std::move(other.m_services))
{
    m_graphicsManagers.init(m_device, m_eventBus, m_taskManager,
                            static_cast<uint8_t>(m_frameInFlightTrackingInfo.getSize()));
    other.m_ownsResources = false;
}

star::core::device::DeviceContext &star::core::device::DeviceContext::operator=(DeviceContext &&other)
{
    if (this != &other)
    {
        m_frameInFlightTrackingInfo = std::move(other.m_frameInFlightTrackingInfo);
        m_frameCounter = std::move(other.m_frameCounter);
        m_deviceID = std::move(other.m_deviceID);
        m_surface = std::move(other.m_surface);
        m_device = std::move(other.m_device);
        m_eventBus = std::move(other.m_eventBus);
        m_taskManager = std::move(other.m_taskManager);
        m_commandBufferManager = std::move(other.m_commandBufferManager);
        m_graphicsManagers = std::move(other.m_graphicsManagers);
        m_transferWorker = std::move(other.m_transferWorker);
        m_renderResourceManager = std::move(other.m_renderResourceManager);
        m_services = std::move(other.m_services);
        if (other.m_ownsResources)
        {
            m_ownsResources = std::move(other.m_ownsResources);

            setAllServiceParameters();
        }

        m_graphicsManagers.init(m_device, m_eventBus, m_taskManager,
                                static_cast<uint8_t>(m_frameInFlightTrackingInfo.getSize()));
        other.m_ownsResources = false;
    }

    return *this;
}

star::core::device::DeviceContext::~DeviceContext()
{
    if (m_ownsResources)
    {
        shutdownServices();
        m_graphicsManagers.cleanupRender();
        m_commandBufferManager->cleanup(*m_device);
        star::ManagerRenderResource::cleanup(m_deviceID, *m_device);
    }
}

void star::core::device::DeviceContext::init(const Handle &deviceID, const uint8_t &numFramesInFlight,
                                             RenderingInstance &instance, std::set<Rendering_Features> requiredFeatures,
                                             StarWindow &window,
                                             const std::set<Rendering_Device_Features> &requiredRenderingDeviceFeatures)
{
    assert(!m_ownsResources && "Dont call init twice");
    assert(deviceID.getType() ==
           common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::DeviceTypeName));
    logInit(numFramesInFlight);

    m_deviceID = deviceID;
    m_frameInFlightTrackingInfo = FrameInFlightTracking(numFramesInFlight);

    m_surface.init(instance, window);
    m_device =
        std::make_shared<StarDevice>(window, m_surface, instance, requiredFeatures, requiredRenderingDeviceFeatures);
    m_commandBufferManager = std::make_unique<manager::ManagerCommandBuffer>(*m_device, numFramesInFlight);
    m_transferWorker = CreateTransferWorker(*m_device);
    m_renderResourceManager = std::make_unique<ManagerRenderResource>();

    initServices(numFramesInFlight);

    initWorkers(numFramesInFlight);

    m_graphicsManagers.init(m_device, m_eventBus, m_taskManager, numFramesInFlight);

    m_ownsResources = true;
}

void star::core::device::DeviceContext::waitIdle()
{
    if (m_ownsResources)
    {
        m_taskManager.cleanup();
        m_transferWorker->stopAll();
    }

    m_device->getVulkanDevice().waitIdle();
}

void star::core::device::DeviceContext::prepareForNextFrame(const uint8_t &frameInFlightIndex)
{
    m_frameInFlightTrackingInfo.getNumOfTimesFrameProcessed(frameInFlightIndex)++;
    m_frameCounter++;

    handleCompleteMessages();
    broadcastFrameStart(frameInFlightIndex);
}

star::core::SwapChainSupportDetails star::core::device::DeviceContext::getSwapchainSupportDetails()
{
    return m_device->getSwapchainSupport(m_surface);
}

std::shared_ptr<star::job::TransferWorker> star::core::device::DeviceContext::CreateTransferWorker(
    StarDevice &device, const size_t &targetNumQueuesToUse)
{
    star::core::logging::log(boost::log::trivial::info, "Initializing transfer workers");

    std::set<uint32_t> selectedFamilyIndices = std::set<uint32_t>();
    std::vector<StarQueue> transferWorkerQueues = std::vector<StarQueue>();

    const auto transferFams =
        device.getQueueOwnershipTracker().getQueueFamiliesWhichSupport(vk::QueueFlagBits::eTransfer);
    for (const auto &fam : transferFams)
    {
        if (fam != device.getDefaultQueue(Queue_Type::Tgraphics).getParentQueueFamilyIndex() &&
            fam != device.getDefaultQueue(Queue_Type::Tcompute).getParentQueueFamilyIndex() &&
            !selectedFamilyIndices.contains(fam))
        {
            auto nQueue =
                device.getQueueOwnershipTracker().giveMeQueueWithProperties(vk::QueueFlagBits::eTransfer, false, fam);

            if (nQueue.has_value())
            {
                transferWorkerQueues.push_back(nQueue.value());

                selectedFamilyIndices.insert(fam);

                // only want 2, one for each of transfer operation
                if (selectedFamilyIndices.size() == targetNumQueuesToUse)
                {
                    break;
                }
            }
        }
    }

    return std::make_shared<job::TransferWorker>(*m_taskManager.getCompleteMessages(), device, false,
                                                 transferWorkerQueues);
}

void star::core::device::DeviceContext::handleCompleteMessages(const uint8_t maxMessagesCounter)
{
    std::vector<job::complete_tasks::CompleteTask> completeMessages;

    if (maxMessagesCounter == 0)
    {
        std::optional<job::complete_tasks::CompleteTask> complete =
            m_taskManager.getCompleteMessages()->getQueuedTask();
        while (complete.has_value())
        {
            processCompleteMessage(std::move(complete.value()));
            complete = m_taskManager.getCompleteMessages()->getQueuedTask();
        }
    }
}

void star::core::device::DeviceContext::processCompleteMessage(job::complete_tasks::CompleteTask completeTask)
{
    completeTask.run(static_cast<void *>(m_device.get()), static_cast<void *>(&m_taskManager),
                     static_cast<void *>(&m_eventBus), static_cast<void *>(&m_graphicsManagers));
}

void star::core::device::DeviceContext::shutdownServices()
{
    for (auto &service : m_services)
    {
        service.shutdown();
    }
}

void star::core::device::DeviceContext::initWorkers(const uint8_t &numFramesInFlight)
{
    ManagerRenderResource::init(m_deviceID, m_device, m_transferWorker, numFramesInFlight);

    const auto transferFams =
        m_device->getQueueOwnershipTracker().getQueueFamiliesWhichSupport(vk::QueueFlagBits::eTransfer);
    std::set<uint32_t> selectedFamilyIndices = std::set<uint32_t>();
    std::vector<StarQueue> transferWorkerQueues = std::vector<StarQueue>();

    for (const auto &fam : transferFams)
    {
        if (fam != m_device->getDefaultQueue(Queue_Type::Tgraphics).getParentQueueFamilyIndex() &&
            fam != m_device->getDefaultQueue(Queue_Type::Tcompute).getParentQueueFamilyIndex() &&
            !selectedFamilyIndices.contains(fam))
        {
            auto nQueue = m_device->getQueueOwnershipTracker().giveMeQueueWithProperties(vk::QueueFlagBits::eTransfer,
                                                                                         false, fam);

            if (nQueue.has_value())
            {
                transferWorkerQueues.push_back(nQueue.value());

                selectedFamilyIndices.insert(fam);
            }
        }
    }

    // create worker for pipeline building
    job::worker::Worker pipelineWorker{job::worker::DefaultWorker{
        job::worker::default_worker::DefaultThreadTaskHandlingPolicy<job::tasks::build_pipeline::BuildPipelineTask,
                                                                     64>{},
        "Pipeline_Builder"}};

    m_taskManager.registerWorker(std::move(pipelineWorker), job::tasks::build_pipeline::BuildPipelineTaskName);

    // create worker for shader compilation
    job::worker::Worker shaderWorker{job::worker::DefaultWorker{
        job::worker::default_worker::DefaultThreadTaskHandlingPolicy<job::tasks::compile_shader::CompileShaderTask,
                                                                     64>{},
        "Shader_Compiler"}};
    m_taskManager.registerWorker(std::move(shaderWorker), job::tasks::compile_shader::CompileShaderTypeName);
}

void star::core::device::DeviceContext::logInit(const uint8_t &numFramesInFlight) const
{
    std::ostringstream oss;
    oss << "Initializing device context: \n \tNumber of frames in flight: " << std::to_string(numFramesInFlight);
    core::logging::log(boost::log::trivial::info, oss.str());
}

void star::core::device::DeviceContext::registerService(service::Service service, const uint8_t &numFramesInFlight)
{
    m_services.emplace_back(std::move(service));

    service::InitParameters params{m_deviceID,
                                   m_surface,
                                   *m_device,
                                   m_eventBus,
                                   m_taskManager,
                                   m_graphicsManagers,
                                   *m_commandBufferManager,
                                   m_frameInFlightTrackingInfo,
                                   *m_transferWorker,
                                   *m_renderResourceManager,
                                   m_frameCounter};

    m_services.back().init(params, numFramesInFlight);
}

void star::core::device::DeviceContext::setAllServiceParameters()
{
    service::InitParameters params{m_deviceID,
                                   m_surface,
                                   *m_device,
                                   m_eventBus,
                                   m_taskManager,
                                   m_graphicsManagers,
                                   *m_commandBufferManager,
                                   m_frameInFlightTrackingInfo,
                                   *m_transferWorker,
                                   *m_renderResourceManager,
                                   m_frameCounter};

    for (auto &service : m_services)
    {
        service.setInitParameters(params);
    }
}

void star::core::device::DeviceContext::initServices(const uint8_t &numOfFramesInFlight)
{
    service::InitParameters params{m_deviceID,
                                   m_surface,
                                   *m_device,
                                   m_eventBus,
                                   m_taskManager,
                                   m_graphicsManagers,
                                   *m_commandBufferManager,
                                   m_frameInFlightTrackingInfo,
                                   *m_transferWorker,
                                   *m_renderResourceManager,
                                   m_frameCounter};

    for (auto &service : m_services)
    {
        service.init(params, numOfFramesInFlight);
    }
}

void star::core::device::DeviceContext::broadcastFrameStart(const uint8_t &frameInFlightIndex)
{
    m_eventBus.emit(star::core::device::system::event::StartOfNextFrame{m_frameCounter, frameInFlightIndex});
}