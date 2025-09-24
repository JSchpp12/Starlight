#include "DeviceContext.hpp"

#include <cassert>

star::core::device::DeviceContext::DeviceContext(
    const uint8_t &numFramesInFlight, const DeviceID &deviceID, RenderingInstance &instance,
    std::set<Rendering_Features> requiredFeatures, StarWindow &window,
    const std::set<Rendering_Device_Features> &requiredRenderingDeviceFeatures)
    : m_deviceID(deviceID), m_surface(RenderingSurface(instance, window)),
      m_device(
          std::make_shared<StarDevice>(window, m_surface, instance, requiredFeatures, requiredRenderingDeviceFeatures)),
      m_commandBufferManager(std::make_unique<managers::ManagerCommandBuffer>(*m_device, numFramesInFlight)),
      m_transferWorker(CreateTransferWorker(*m_device)),
      m_renderResourceManager(std::make_unique<ManagerRenderResource>())
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

    m_taskManager.registerWorker(typeid(star::job::tasks::ImageWritePayload), std::make_shared<star::job::worker::Worker<512>>()); 

    m_taskManager.startAll();
}

star::core::device::DeviceContext::~DeviceContext()
{
    if (m_ownsResources)
    {
        m_taskManager.stopAll();
        m_graphicsManagers.pipelineManager.cleanupRender(*m_device); 
        m_graphicsManagers.shaderManager.cleanupRender(*m_device); 
        m_commandBufferManager->cleanup(*m_device);

        ManagerRenderResource::cleanup(m_deviceID, *m_device); 
    }
}

void star::core::device::DeviceContext::waitIdle(){
    m_taskManager.stopAll(); 
    m_device->getVulkanDevice().waitIdle(); 
}

void star::core::device::DeviceContext::prepareForNextFrame()
{
    handleCompleteMessages();

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