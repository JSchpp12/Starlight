#include "DeviceContext.hpp"

star::core::SwapChainSupportDetails star::core::device::DeviceContext::getSwapchainSupportDetails()
{
    return m_device.getSwapchainSupport(*m_surface);
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