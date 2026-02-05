#include "core/device/DeviceContext.hpp"

#include "core/logging/LoggingFactory.hpp"
#include "event/PrepForNextFrame.hpp"
#include "event/StartOfNextFrame.hpp"
#include "job/tasks/TaskFactory.hpp"
#include "job/worker/DefaultWorker.hpp"
#include "job/worker/Worker.hpp"
#include "managers/ManagerRenderResource.hpp"
#include "service/InitParameters.hpp"
#include "service/QueueManagerService.hpp"
#include "starlight/core/helper/queue/QueueHelpers.hpp"
#include "starlight/event/GetQueue.hpp"
#include "starlight/job/worker/detail/default_worker/BusyWaitTaskHandlingPolicy.hpp"
#include "starlight/service/SceneLoaderService.hpp"
#include "starlight/wrappers/graphics/QueueFamilyIndices.hpp"

#include <star_common/HandleTypeRegistry.hpp>
#include <star_common/helper/CastHelpers.hpp>

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
    if (m_commandBufferManager)
    {
        m_commandBufferManager->init(m_graphicsManagers.queueManager);
    }

    other.m_ownsResources = false;
}

void star::core::device::DeviceContext::submit(star::common::IServiceCommand &command)
{
    m_commandBus.submit(command);
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
        if (m_commandBufferManager)
        {
            m_commandBufferManager->init(m_graphicsManagers.queueManager);
        }

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
        m_graphicsManagers.cleanupRender();
        m_commandBufferManager->cleanup(m_device);
        star::ManagerRenderResource::cleanup(m_deviceID, m_device);
        m_eventBus.cleanup();
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

    m_graphicsManagers.init(&m_device, m_eventBus, m_taskManager, m_flightTracker.getSetup().getNumFramesInFlight());
    auto availableQueues = processAvailableQueues();
    auto engineReservedQueues = selectEngineReservedQueues(availableQueues);
    m_commandBufferManager = std::make_unique<core::device::manager::ManagerCommandBuffer>(
        m_device, m_graphicsManagers.queueManager, m_flightTracker.getSetup().getNumFramesInFlight(),
        engineReservedQueues);
    m_commandBufferManager->init(m_graphicsManagers.queueManager);
    m_renderResourceManager = std::make_unique<ManagerRenderResource>();

    m_services.emplace_back(createQueueOwnershipService(availableQueues, engineReservedQueues));
    m_services.emplace_back(createSceneLoaderService());
    initServices(m_flightTracker.getSetup().getNumFramesInFlight());

    m_transferWorker = createTransferWorker(m_device, engineReservedQueues);
    initWorkers(m_flightTracker.getSetup().getNumFramesInFlight());

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

    broadcastFramePrepToService();

    // assuming external service will handle updating the tracker
    m_flightTracker.triggerIncrementForCurrentFrame();
    broadcastFrameStart();
}

void star::core::device::DeviceContext::cleanupRender()
{
    shutdownServices();
}

std::unordered_set<uint32_t> star::core::device::DeviceContext::gatherEngineDedicatedQueueFamilyIndices()
{
    std::unordered_set<uint32_t> dedicated;

    const auto *graphicsQueue =
        core::helper::GetEngineDefaultQueue(m_eventBus, m_graphicsManagers.queueManager, star::Queue_Type::Tgraphics);

    return dedicated;
}

std::vector<star::Handle> star::core::device::DeviceContext::gatherTransferQueues(
    const uint8_t &targetNumberOfQueues) const
{
    std::vector<star::Handle> targetQueues;

    return targetQueues;
}

std::shared_ptr<star::job::TransferWorker> star::core::device::DeviceContext::createTransferWorker(
    StarDevice &device, absl::flat_hash_map<star::Queue_Type, Handle> engineReserved,
    const size_t &targetNumQueuesToUse)
{
    star::core::logging::log(boost::log::trivial::info, "Initializing transfer workers");

    std::set<uint32_t> selectedFamilyIndices = std::set<uint32_t>();
    std::vector<StarQueue *> transferWorkerQueues = std::vector<StarQueue *>(targetNumQueuesToUse);

    auto selectedTransferFamilyIndex =
        m_graphicsManagers.queueManager.get(engineReserved.at(star::Queue_Type::Ttransfer))
            ->queue.getParentQueueFamilyIndex();

    Handle queue;
    for (size_t i{0}; i < targetNumQueuesToUse; i++)
    {
        m_eventBus.emit(event::GetQueue::Builder()
                            .setQueueData(queue)
                            .setQueueType(star::Queue_Type::Ttransfer)
                            .setSelectFromFamilyIndex(selectedTransferFamilyIndex)
                            .build());

        if (queue.isInitialized())
        {
            transferWorkerQueues[i] = &m_graphicsManagers.queueManager.get(queue)->queue;
        }
        else
        {
            break;
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
    // start from the back as later services might rely on earlier "core" services which are registered first

    for (size_t i{m_services.size()}; i > 0; i--)
    {
        m_services[i - 1].shutdown();
    }
}

void star::core::device::DeviceContext::initWorkers(const uint8_t &numFramesInFlight)
{
    ManagerRenderResource::init(m_deviceID, &m_device, m_transferWorker, numFramesInFlight);

    // create worker for pipeline building
    job::worker::Worker pipelineWorker{job::worker::DefaultWorker{
        job::worker::default_worker::BusyWaitTaskHandlingPolicy<job::tasks::build_pipeline::BuildPipelineTask, 64>{},
        "Pipeline_Builder"}};

    m_taskManager.registerWorker(std::move(pipelineWorker), job::tasks::build_pipeline::BuildPipelineTaskName);

    // create worker for shader compilation
    job::worker::Worker shaderWorker{job::worker::DefaultWorker{
        job::worker::default_worker::BusyWaitTaskHandlingPolicy<job::tasks::compile_shader::CompileShaderTask, 64>{},
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

    service::InitParameters params{begin(),
                                   m_deviceID,
                                   m_device,
                                   m_eventBus,
                                   m_commandBus,
                                   m_taskManager,
                                   m_graphicsManagers,
                                   *m_commandBufferManager,
                                   *m_renderResourceManager,
                                   m_flightTracker};

    m_services.back().init(params);
}

void star::core::device::DeviceContext::setAllServiceParameters()
{
    service::InitParameters params{begin(),
                                   m_deviceID,
                                   m_device,
                                   m_eventBus,
                                   m_commandBus,
                                   m_taskManager,
                                   m_graphicsManagers,
                                   *m_commandBufferManager,
                                   *m_renderResourceManager,
                                   m_flightTracker};

    for (auto &service : m_services)
    {
        service.setInitParameters(params);
    }
}

void star::core::device::DeviceContext::initServices(const uint8_t &numOfFramesInFlight)
{
    service::InitParameters params{begin(),
                                   m_deviceID,
                                   m_device,
                                   m_eventBus,
                                   m_commandBus,
                                   m_taskManager,
                                   m_graphicsManagers,
                                   *m_commandBufferManager,
                                   *m_renderResourceManager,
                                   m_flightTracker};

    for (auto &service : m_services)
    {
        service.init(params);
    }
}

std::vector<star::Handle> star::core::device::DeviceContext::processAvailableQueues()
{
    QueueFamilyIndices indices = m_device.getQueueInfo();
    std::vector<star::Handle> handles;

    auto families = indices.getQueueFamilies();
    for (auto &family : families)
    {
        family.init(m_device.getVulkanDevice());

        for (auto &queue : family.getQueues())
        {
            auto record = m_graphicsManagers.queueManager.submit({queue});
            handles.emplace_back(std::move(record));
        }
    }

    return handles;
}

void star::core::device::DeviceContext::broadcastFrameStart()
{
    m_eventBus.emit(event::StartOfNextFrame{m_flightTracker});
}

void star::core::device::DeviceContext::broadcastFramePrepToService()
{
    m_eventBus.emit(star::event::PrepForNextFrame{m_flightTracker});
}

static vk::QueueFlags EnumToQueueFlags(const star::Queue_Type &type)
{
    switch (type)
    {
    case (star::Queue_Type::Tgraphics):
        return vk::QueueFlagBits::eGraphics;
        break;
    case (star::Queue_Type::Ttransfer):
        return vk::QueueFlagBits::eTransfer;
        break;
    case (star::Queue_Type::Tcompute):
        return vk::QueueFlagBits::eCompute;
        break;
    default:
        return vk::QueueFlags{};
    }
}

star::Handle star::core::device::DeviceContext::getQueueOfType(
    const std::vector<Handle> &allQueueHandles, const star::Queue_Type &type,
    const std::unordered_set<uint32_t> *queueFamilyIndsToAvoid)
{
    for (const auto &handle : allQueueHandles)
    {
        const auto &queue = m_graphicsManagers.queueManager.get(handle)->queue;
        if (queueFamilyIndsToAvoid == nullptr ||
            (queueFamilyIndsToAvoid != nullptr && !queueFamilyIndsToAvoid->contains(queue.getParentQueueFamilyIndex())))
        {
            if (type == star::Queue_Type::Tpresent && queue.getDoesSupportPresentation())
            {
                return handle;
            }
            else if (queue.isCompatibleWith(EnumToQueueFlags(type)))
            {
                return handle;
            }
        }
    }

    return Handle();
}

absl::flat_hash_map<star::Queue_Type, star::Handle> star::core::device::DeviceContext::selectEngineReservedQueues(
    const std::vector<star::Handle> &allQueueHandles)
{
    // const auto type = common::HandleTypeRegistry::instance().getType(common::special_types::F)
    //  select a queue of each type
    absl::flat_hash_map<star::Queue_Type, star::Handle> selectedQueues;

    // start with present becuase that one is special for non-headless environments
    std::unordered_set<uint32_t> selectedFamilyInds;
    Handle selected = getQueueOfType(allQueueHandles, star::Queue_Type::Tpresent, nullptr);

    if (selected.isInitialized())
    {
        selectedQueues.insert(
            std::make_pair<star::Queue_Type, star::Handle>(star::Queue_Type::Tpresent, Handle(selected)));
        selectedFamilyInds.insert(m_graphicsManagers.queueManager.get(selected)->queue.getParentQueueFamilyIndex());
    }

    // try and select graphics shared with present if present
    if (!selectedQueues.empty())
    {
        // check if the present queue supports graphics
        // Handle handle = selectedQueues[star::Queue_Type::Tgraphics];
        if (m_graphicsManagers.queueManager.get(selected)->queue.isCompatibleWith(
                EnumToQueueFlags(star::Queue_Type::Tgraphics)))
        {
            selectedQueues.insert(
                std::make_pair<star::Queue_Type, star::Handle>(star::Queue_Type::Tgraphics, Handle(selected)));
        }
        else
        {
            STAR_THROW("Not able to find queue which supports graphics or presentation");
        }
    }
    else
    {
        selected = getQueueOfType(allQueueHandles, star::Queue_Type::Tgraphics, nullptr);
        // dedicated graphics family
        selectedQueues.insert(
            std::make_pair<star::Queue_Type, star::Handle>(star::Queue_Type::Tgraphics, Handle(selected)));
        selectedFamilyInds.insert(m_graphicsManagers.queueManager.get(selected)->queue.getParentQueueFamilyIndex());
    }

    // try to get dedicated transfer queue
    selected = getQueueOfType(allQueueHandles, star::Queue_Type::Ttransfer, &selectedFamilyInds);
    if (selected.isInitialized())
    {
        selectedQueues.insert(
            std::make_pair<star::Queue_Type, star::Handle>(star::Queue_Type::Ttransfer, Handle(selected)));
        selectedFamilyInds.insert(m_graphicsManagers.queueManager.get(selected)->queue.getParentQueueFamilyIndex());
    }
    else
    {
        // use selected graphics queue
        selectedQueues.insert(std::make_pair<star::Queue_Type, star::Handle>(
            star::Queue_Type::Ttransfer, Handle(selectedQueues[star::Queue_Type::Tgraphics])));
    }

    // try to get dedicated compute queue
    selected = getQueueOfType(allQueueHandles, star::Queue_Type::Tcompute, &selectedFamilyInds);
    if (selected.isInitialized())
    {
        auto pair = std::make_pair<star::Queue_Type, star::Handle>(star::Queue_Type::Tcompute, Handle(selected));
        selectedQueues.insert(std::move(pair));
        selectedFamilyInds.insert(m_graphicsManagers.queueManager.get(selected)->queue.getParentQueueFamilyIndex());
    }
    else
    {
        // vulkan guarantees one queue which supports all
        selectedQueues.insert(std::make_pair<star::Queue_Type, star::Handle>(
            star::Queue_Type::Tcompute, Handle(selectedQueues[star::Queue_Type::Tgraphics])));
    }

    return selectedQueues;
}

star::service::Service star::core::device::DeviceContext::createQueueOwnershipService(
    std::vector<Handle> queueHandles, absl::flat_hash_map<star::Queue_Type, Handle> engineReserved)
{
    auto service = service::QueueManagerService(std::move(queueHandles), std::move(engineReserved));
    return star::service::Service(std::move(service));
}

star::service::Service star::core::device::DeviceContext::createSceneLoaderService()
{
    auto service = service::SceneLoaderService();
    return star::service::Service(std::move(service));
}