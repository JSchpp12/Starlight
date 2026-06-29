#include "job/TransferWorker.hpp"

#include "core/Exceptions.hpp"
#include "logging/LoggingFactory.hpp"

#include <sstream>

namespace star::job
{

void TransferManagerThread::CreateBuffer(vk::Device device, VmaAllocator allocator, StarQueue &queue,
                                         vk::Semaphore signalWhenDoneSemaphore,
                                         const vk::PhysicalDeviceProperties &deviceProperties,
                                         const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                                         ProcessRequestInfo &processInfo, TransferRequest::Buffer *newBufferRequest,
                                         std::unique_ptr<StarBuffers::Buffer> *resultingBuffer,
                                         boost::atomic<bool> *gpuDoneSignalMain,
                                         std::optional<core::graphics::GPUWorkSyncInfo> &syncInfo)
{
    auto transferSrcBuffer = newBufferRequest->createStagingBuffer(device, allocator);
    if (transferSrcBuffer->getBufferSize() == 0)
        STAR_THROW("Failed to create transfer src buffer");

    {
        auto newResult = newBufferRequest->createFinal(device, allocator, allTransferQueueFamilyIndicesInUse);
        if (newResult->getBufferSize() == 0)
        {
            STAR_THROW("Failed to create final buffer");
        }
        if (!*resultingBuffer ||
            (resultingBuffer->get() && newResult->getBufferSize() > resultingBuffer->get()->getBufferSize()))
        {
            resultingBuffer->swap(newResult);
        }
    }

    newBufferRequest->writeDataToStageBuffer(*transferSrcBuffer);

    newBufferRequest->copyFromTransferSRCToDST(*transferSrcBuffer, *resultingBuffer->get(),
                                               processInfo.commandBuffer->buffer(0));

    processInfo.commandBuffer->buffer(0).end();

    const auto signalInfo = vk::SemaphoreSubmitInfo()
                                .setSemaphore(signalWhenDoneSemaphore)
                                .setValue(0)
                                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

    uint8_t waitInfoCount{0};
    vk::SemaphoreSubmitInfo waitInfo;
    if (syncInfo.has_value())
    {
        const auto &v = syncInfo.value();
        waitInfo.setSemaphore(v.workWaitOn.semaphore)
            .setValue(v.workWaitOn.signalValue)
            .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
        waitInfoCount++;
    }

    const auto cbInfo = vk::CommandBufferSubmitInfo().setCommandBuffer(processInfo.commandBuffer->buffer(0));

    const auto submitInfo = vk::SubmitInfo2()
                                .setPWaitSemaphoreInfos(&waitInfo)
                                .setWaitSemaphoreInfoCount(waitInfoCount)
                                .setPCommandBufferInfos(&cbInfo)
                                .setCommandBufferInfoCount(1)
                                .setPSignalSemaphoreInfos(&signalInfo)
                                .setSignalSemaphoreInfoCount(1);

    try
    {
        queue.getVulkanQueue().submit2(submitInfo, processInfo.commandBuffer->getFence(0));
    }
    catch (const vk::Error &e)
    {
        std::ostringstream oss;
        oss << "Vulkan error encountered while submitting queue. Terminating. " << e.what();
        STAR_THROW(oss.str());
    }

    processInfo.setInProcessDeps(std::move(transferSrcBuffer));
}

void TransferManagerThread::CreateTexture(vk::Device device, VmaAllocator allocator, StarQueue &queue,
                                          vk::Semaphore signalWhenDoneSemaphore,
                                          const vk::PhysicalDeviceProperties &deviceProperties,
                                          const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                                          ProcessRequestInfo &processInfo, TransferRequest::Texture *newTextureRequest,
                                          std::unique_ptr<StarTextures::Texture> *resultingTexture,
                                          boost::atomic<bool> *gpuDoneSignalToMain)
{
    auto transferSrcBuffer = newTextureRequest->createStagingBuffer(device, allocator);

    bool newImageCreated = true;

    auto finalTexture = newTextureRequest->createFinal(device, allocator, allTransferQueueFamilyIndicesInUse);
    resultingTexture->swap(finalTexture);

    newTextureRequest->writeDataToStageBuffer(*transferSrcBuffer);

    if (!newImageCreated)
    {
        STAR_THROW("Unsupported image operation in transfer manager");
    }

    newTextureRequest->copyFromTransferSRCToDST(*transferSrcBuffer, *resultingTexture->get(),
                                                processInfo.commandBuffer->buffer(0));

    processInfo.commandBuffer->buffer(0).end();
    auto signalSemaphores = std::vector<vk::Semaphore>{signalWhenDoneSemaphore};
    try
    {
        processInfo.commandBuffer->submit(0, queue.getVulkanQueue(), nullptr, nullptr, nullptr, &signalSemaphores);
    }
    catch (const vk::Error &e)
    {
        std::ostringstream oss;
        oss << "Vulkan error encountered while submitting queue. Terminating. " << e.what();
        STAR_THROW(oss.str());
    }

    processInfo.setInProcessDeps(std::move(transferSrcBuffer));
}

void TransferManagerThread::CheckForCleanups(vk::Device device,
                                             std::queue<std::unique_ptr<ProcessRequestInfo>> &processingInfos)
{
    std::queue<std::unique_ptr<ProcessRequestInfo>> readyInfos;
    std::queue<std::unique_ptr<ProcessRequestInfo>> notReadyInfos;

    while (!processingInfos.empty())
    {
        std::unique_ptr<ProcessRequestInfo> workingInfo = std::move(processingInfos.front());
        processingInfos.pop();

        if (!workingInfo->isMarkedAsAvailable() && workingInfo->commandBuffer->isFenceReady(0))
        {
            workingInfo->markAsAvailable(device);
            readyInfos.push(std::move(workingInfo));
        }
        else
        {
            notReadyInfos.push(std::move(workingInfo));
        }
    }

    while (!readyInfos.empty())
    {
        processingInfos.push(std::move(readyInfos.front()));
        readyInfos.pop();
    }

    while (!notReadyInfos.empty())
    {
        processingInfos.push(std::move(notReadyInfos.front()));
        notReadyInfos.pop();
    }
}

} // namespace star::job
