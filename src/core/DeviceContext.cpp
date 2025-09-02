#include "DeviceContext.hpp"

star::core::device::DeviceContext::DeviceContext(const DeviceID &deviceID, 
    const uint8_t &numFramesInFlight, RenderingInstance &instance, std::set<Rendering_Features> requiredFeatures,
    StarWindow &window, const std::set<Rendering_Device_Features> &requiredRenderingDeviceFeatures)
    : m_deviceID(deviceID), m_surface(std::make_shared<RenderingSurface>(instance, window)),
      m_device(std::make_shared<StarDevice>(window, *m_surface, instance, requiredFeatures, requiredRenderingDeviceFeatures)),
      m_commandBufferManager(std::make_unique<managers::ManagerCommandBuffer>(*m_device, numFramesInFlight)),
      m_transferWorker(CreateTransferWorker(*m_device)),
      m_renderResourceManager(std::make_unique<ManagerRenderResource>())
{
    ManagerRenderResource::init(m_deviceID, m_device, *m_transferWorker, numFramesInFlight);

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
            auto nQueue =
                m_device->getQueueOwnershipTracker().giveMeQueueWithProperties(vk::QueueFlagBits::eTransfer, false, fam);

            if (nQueue.has_value())
            {
                transferWorkerQueues.push_back(nQueue.value());

                selectedFamilyIndices.insert(fam);
            }
        }
    }

    auto &newWorker = m_manager.registerWorker(typeid(star::job::tasks::IODataPreparationPayload));

    m_manager.startAll();

    // vk::SemaphoreTypeCreateInfo timelineCreateInfo{};
    // timelineCreateInfo.sType = vk::StructureType::eSemaphoreTypeCreateInfo;
    // timelineCreateInfo.pNext = NULL;
    // timelineCreateInfo.semaphoreType = vk::SemaphoreType::eTimeline;
    // timelineCreateInfo.initialValue = 0;

    // vk::SemaphoreCreateInfo create{};
    // create.sType = vk::StructureType::eSemaphoreCreateInfo;
    // create.pNext = &timelineCreateInfo;
    // vk::Semaphore newSemaphore = m_device.getVulkanDevice().createSemaphore(create);

    // std::string path =
    //     star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "terrains/1805583_s2025470.ktx2";
    // newWorker.queueTask(
    //     job::tasks::task_factory::createTextureTransferTask(path, m_device.getPhysicalDevice(), newSemaphore));

    // // for testing only

    // job::complete_tasks::CompleteTask<> completeTask;
    // while (!m_manager.getCompleteMessages()->pop(completeTask))
    // {
    // }

    // completeTask.run(&m_device);

    m_manager.stopAll();
}

star::core::SwapChainSupportDetails star::core::device::DeviceContext::getSwapchainSupportDetails()
{
    return m_device->getSwapchainSupport(*m_surface);
}

std::unique_ptr<star::job::TransferWorker> star::core::device::DeviceContext::CreateTransferWorker(StarDevice &device)
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

    return std::make_unique<job::TransferWorker>(device, false, transferWorkerQueues);
}