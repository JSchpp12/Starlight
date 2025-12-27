#include "core/device/DeviceContext.hpp"

#include "core/logging/LoggingFactory.hpp"
#include "job/tasks/TaskFactory.hpp"
#include "job/worker/DefaultWorker.hpp"
#include "job/worker/Worker.hpp"
#include "job/worker/default_worker/detail/ThreadTaskHandlingPolicies.hpp"

#include "core/device/system/event/StartOfNextFrame.hpp"
#include "event/PrepForNextFrame.hpp"
#include "managers/ManagerRenderResource.hpp"

#include <star_common/HandleTypeRegistry.hpp>
#include <star_common/helper/CastHelpers.hpp>

#include "service/InitParameters.hpp"

#include <cassert>

star::core::device::DeviceContext::DeviceContext(DeviceContext &&other)
    : m_device(std::move(other.m_device)), m_flightTracker(std::move(other.m_flightTracker)),
      m_deviceID(std::move(other.m_deviceID)), m_eventBus(), m_taskManager(std::move(other.m_taskManager)),
      m_graphicsManagers(), m_commandBufferManager(std::move(other.m_commandBufferManager)),
      m_transferWorker(std::move(other.m_transferWorker)),
      m_renderResourceManager(std::move(other.m_renderResourceManager)), m_services(std::move(other.m_services))
{
    m_graphicsManagers = std::move(other.m_graphicsManagers);
    m_eventBus = std::move(other.m_eventBus);

    m_graphicsManagers.init(&m_device, m_eventBus, m_taskManager,
                            static_cast<uint8_t>(m_flightTracker.getSetup().getNumFramesInFlight()));

    other.m_ownsResources = false;
}

star::core::device::DeviceContext &star::core::device::DeviceContext::operator=(DeviceContext &&other)
{
    if (this != &other)
    {
        m_flightTracker = std::move(other.m_flightTracker);
        m_deviceID = std::move(other.m_deviceID);
        m_device = std::move(other.m_device);
        m_taskManager = std::move(other.m_taskManager);
        m_commandBufferManager = std::move(other.m_commandBufferManager);
        m_transferWorker = std::move(other.m_transferWorker);
        m_renderResourceManager = std::move(other.m_renderResourceManager);
        m_services = std::move(other.m_services);

        m_graphicsManagers = std::move(other.m_graphicsManagers);
        m_eventBus = std::move(other.m_eventBus);
        m_graphicsManagers.init(&m_device, m_eventBus, m_taskManager,
                                static_cast<uint8_t>(m_flightTracker.getSetup().getNumFramesInFlight()));

        if (other.m_ownsResources)
        {
            m_ownsResources = std::move(other.m_ownsResources);

            setAllServiceParameters();
        }

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
        m_commandBufferManager->cleanup(m_device);
        star::ManagerRenderResource::cleanup(m_deviceID, m_device);
    }
}

void star::core::device::DeviceContext::init(const Handle &deviceID, common::FrameTracker::Setup setup,
                                             vk::Extent2D engineResolution)
{
    assert(!m_ownsResources && "Dont call init twice");
    assert(deviceID.getType() ==
           common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::DeviceTypeName));
    m_flightTracker = common::FrameTracker(std::move(setup));

    logInit(m_flightTracker.getSetup().getNumFramesInFlight());
    m_deviceID = deviceID;
    m_engineResolution = std::move(engineResolution);

    m_commandBufferManager =
        std::make_unique<manager::ManagerCommandBuffer>(m_device, m_flightTracker.getSetup().getNumFramesInFlight());
    m_transferWorker = CreateTransferWorker(m_device);
    m_renderResourceManager = std::make_unique<ManagerRenderResource>();

    initServices(m_flightTracker.getSetup().getNumFramesInFlight());

    initWorkers(m_flightTracker.getSetup().getNumFramesInFlight());

    m_graphicsManagers.init(&m_device, m_eventBus, m_taskManager, m_flightTracker.getSetup().getNumFramesInFlight());

    m_ownsResources = true;
}

void star::core::device::DeviceContext::waitIdle()
{
    if (m_ownsResources)
    {
        m_taskManager.cleanup();
        m_transferWorker->stopAll();
    }

    m_device.getVulkanDevice().waitIdle();
}

void star::core::device::DeviceContext::prepareForNextFrame()
{
    handleCompleteMessages();

    uint8_t currentFrame;
    broadcastFramePrepToService();

    // assuming external service will handle updating the tracker
    m_flightTracker.triggerIncrementForCurrentFrame();
    broadcastFrameStart();
}

void star::core::device::DeviceContext::cleanupRender()
{
    shutdownServices();
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
    completeTask.run(static_cast<void *>(&m_device), static_cast<void *>(&m_taskManager),
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
    ManagerRenderResource::init(m_deviceID, &m_device, m_transferWorker, numFramesInFlight);

    const auto transferFams =
        m_device.getQueueOwnershipTracker().getQueueFamiliesWhichSupport(vk::QueueFlagBits::eTransfer);
    std::set<uint32_t> selectedFamilyIndices = std::set<uint32_t>();
    std::vector<StarQueue> transferWorkerQueues = std::vector<StarQueue>();

    for (const auto &fam : transferFams)
    {
        if (fam != m_device.getDefaultQueue(Queue_Type::Tgraphics).getParentQueueFamilyIndex() &&
            fam != m_device.getDefaultQueue(Queue_Type::Tcompute).getParentQueueFamilyIndex() &&
            !selectedFamilyIndices.contains(fam))
        {
            auto nQueue =
                m_device.getQueueOwnershipTracker().giveMeQueueWithProperties(vk::QueueFlagBits::eTransfer, false, fam);

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

void star::core::device::DeviceContext::registerService(service::Service service)
{
    m_services.emplace_back(std::move(service));

    service::InitParameters params{m_deviceID,         m_device,
                                   m_eventBus,         m_taskManager,
                                   m_graphicsManagers, *m_commandBufferManager,
                                   *m_transferWorker,  *m_renderResourceManager,
                                   m_flightTracker};

    m_services.back().init(params, m_flightTracker.getSetup().getNumFramesInFlight());
}

void star::core::device::DeviceContext::setAllServiceParameters()
{
    service::InitParameters params{m_deviceID,         m_device,
                                   m_eventBus,         m_taskManager,
                                   m_graphicsManagers, *m_commandBufferManager,
                                   *m_transferWorker,  *m_renderResourceManager,
                                   m_flightTracker};

    for (auto &service : m_services)
    {
        service.setInitParameters(params);
    }
}

void star::core::device::DeviceContext::initServices(const uint8_t &numOfFramesInFlight)
{
    service::InitParameters params{m_deviceID,         m_device,
                                   m_eventBus,         m_taskManager,
                                   m_graphicsManagers, *m_commandBufferManager,
                                   *m_transferWorker,  *m_renderResourceManager,
                                   m_flightTracker};

    for (auto &service : m_services)
    {
        service.init(params, numOfFramesInFlight);
    }
}

void star::core::device::DeviceContext::broadcastFrameStart()
{
    m_eventBus.emit(star::core::device::system::event::StartOfNextFrame{
        m_flightTracker.getCurrent().getGlobalFrameCounter(), m_flightTracker.getCurrent().getFrameInFlightIndex()});
}

void star::core::device::DeviceContext::broadcastFramePrepToService()
{
    m_eventBus.emit(star::event::PrepForNextFrame{m_flightTracker});
}