#include "core/device/DeviceContext.hpp"

#include <cassert>

star::core::device::DeviceContext::~DeviceContext()
{
    if (m_ownsResources)
    {
        m_graphicsManagers.fenceManager->cleanupRender(*m_device);
        m_graphicsManagers.pipelineManager->cleanupRender(*m_device);
        m_graphicsManagers.shaderManager->cleanupRender(*m_device);
        m_graphicsManagers.semaphoreManager->cleanupRender(*m_device);
        m_commandBufferManager->cleanup(*m_device);

        ManagerRenderResource::cleanup(m_deviceID, *m_device);
    }
}

void star::core::device::DeviceContext::init(const Handle &deviceID, const uint8_t &numFramesInFlight,
                                             RenderingInstance &instance, std::set<Rendering_Features> requiredFeatures,
                                             StarWindow &window,
                                             const std::set<Rendering_Device_Features> &requiredRenderingDeviceFeatures)
{
    assert(!m_ownsResources && "Dont call init twice");
    assert(deviceID.getType() == Handle_Type::device);

    m_deviceID = deviceID;

    m_frameInFlightTrackingInfo.resize(numFramesInFlight);

    m_surface.init(instance, window);
    m_device =
        std::make_shared<StarDevice>(window, m_surface, instance, requiredFeatures, requiredRenderingDeviceFeatures);
    m_commandBufferManager = std::make_unique<manager::ManagerCommandBuffer>(*m_device, numFramesInFlight);
    m_transferWorker = CreateTransferWorker(*m_device);
    m_renderResourceManager = std::make_unique<ManagerRenderResource>();

    initWorkers(numFramesInFlight);

    m_graphicsManagers.init(m_device, m_eventBus);

    m_ownsResources = true;
}

void star::core::device::DeviceContext::waitIdle()
{
    if (m_ownsResources)
    {
        m_taskManager.stopAll();
        m_transferWorker->stopAll();
    }

    m_device->getVulkanDevice().waitIdle();
}

void star::core::device::DeviceContext::prepareForNextFrame(const uint8_t &frameInFlightIndex)
{
    assert(frameInFlightIndex < m_frameInFlightTrackingInfo.size());

    handleCompleteMessages();

    m_frameInFlightTrackingInfo[frameInFlightIndex].numOfTimesFrameProcessed++;
    m_frameCounter++;
}

star::core::SwapChainSupportDetails star::core::device::DeviceContext::getSwapchainSupportDetails()
{
    return m_device->getSwapchainSupport(m_surface);
}

std::shared_ptr<star::job::TransferWorker> star::core::device::DeviceContext::CreateTransferWorker(StarDevice &device)
{
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
                if (selectedFamilyIndices.size() == 1)
                {
                    break;
                }
            }
        }
    }

    return std::make_shared<job::TransferWorker>(device, false, transferWorkerQueues);
}

void star::core::device::DeviceContext::handleCompleteMessages(const uint8_t maxMessagesCounter)
{
    std::vector<job::complete_tasks::CompleteTask<>> completeMessages;

    if (maxMessagesCounter == 0)
    {
        size_t count = m_taskManager.getCompleteMessages()->consume_all(
            [this](auto msg) { processCompleteMessage(std::move(msg)); });
    }
}

void star::core::device::DeviceContext::processCompleteMessage(job::complete_tasks::CompleteTask<> completeTask)
{
    completeTask.run(static_cast<void *>(m_device.get()), static_cast<void *>(&m_taskManager),
                     static_cast<void *>(&m_eventBus), static_cast<void *>(&m_graphicsManagers));
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

    m_taskManager.registerWorker(typeid(star::job::tasks::ImageWritePayload),
                                 std::make_shared<star::job::worker::Worker<512>>());

    m_taskManager.startAll();
}