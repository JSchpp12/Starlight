#include "job/TransferWorker.hpp"

#include "core/Exceptions.hpp"
#include "logging/LoggingFactory.hpp"

#include <sstream>

namespace star::job
{

void TransferManagerThread::CreateBuffer(core::device::StarDevice &device, StarQueue &queue,
                                         const vk::PhysicalDeviceProperties &deviceProperties,
                                         const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                                         ProcessRequestInfo &processInfo, TransferRequest::Buffer *newBufferRequest,
                                         std::unique_ptr<StarBuffers::Buffer> *resultingBuffer,
                                         boost::atomic<bool> *gpuDoneSignalMain,
                                         core::graphics::GPUWorkSyncInfo &syncInfo)
{
    auto transferSrcBuffer = newBufferRequest->createStagingBuffer(device);
    if (transferSrcBuffer->getBufferSize() == 0)
        STAR_THROW("Failed to create transfer src buffer");

    {
        auto newResult = newBufferRequest->createFinal(device, allTransferQueueFamilyIndicesInUse);
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

    {
        const auto signalInfo = vk::SemaphoreSubmitInfo()
                                    .setSemaphore(syncInfo.workSignalWhenDone.semaphore)
                                    .setValue(syncInfo.workSignalWhenDone.signalValue)
                                    .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

        std::array<vk::SemaphoreSubmitInfo, 2> waitInfos{};
        for (uint8_t i = 0; i < syncInfo.workWaitOnCount; ++i)
        {
            const auto &wait = syncInfo.workWaitOn[i];
            waitInfos[i].setSemaphore(wait.semaphore).setValue(wait.signalValue).setStageMask(wait.where);
        }

        const auto cbInfo = vk::CommandBufferSubmitInfo().setCommandBuffer(processInfo.commandBuffer->buffer(0));

        const auto submitInfo = vk::SubmitInfo2()
                                    .setPWaitSemaphoreInfos(waitInfos.data())
                                    .setWaitSemaphoreInfoCount(syncInfo.workWaitOnCount)
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
    }

    processInfo.commandBuffer->markSubmitted();
    processInfo.setInProcessDeps(std::move(transferSrcBuffer));
}

void TransferManagerThread::CreateTexture(core::device::StarDevice &device, StarQueue &queue,
                                          const vk::PhysicalDeviceProperties &deviceProperties,
                                          const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                                          ProcessRequestInfo &processInfo, TransferRequest::Texture *newTextureRequest,
                                          std::optional<TextureTransferPromise> &textureSlot,
                                          boost::atomic<bool> *gpuDoneSignalToMain,
                                          core::graphics::GPUWorkSyncInfo &syncInfo)
{
    auto transferSrcBuffer = newTextureRequest->createStagingBuffer(device);

    bool newImageCreated = true;

    auto finalTexture = newTextureRequest->createFinal(device, allTransferQueueFamilyIndicesInUse);

    newTextureRequest->writeDataToStageBuffer(*transferSrcBuffer);

    if (!newImageCreated)
    {
        STAR_THROW("Unsupported image operation in transfer manager");
    }

    newTextureRequest->copyFromTransferSRCToDST(*transferSrcBuffer, *finalTexture,
                                                processInfo.commandBuffer->buffer(0));

    processInfo.commandBuffer->buffer(0).end();

    {
        const auto signalInfo = vk::SemaphoreSubmitInfo()
                                    .setSemaphore(syncInfo.workSignalWhenDone.semaphore)
                                    .setValue(syncInfo.workSignalWhenDone.signalValue)
                                    .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

        std::array<vk::SemaphoreSubmitInfo, 2> waitInfos{};
        for (uint8_t i = 0; i < syncInfo.workWaitOnCount; ++i)
        {
            const auto &wait = syncInfo.workWaitOn[i];
            waitInfos[i].setSemaphore(wait.semaphore).setValue(wait.signalValue).setStageMask(wait.where);
        }

        const auto cbInfo = vk::CommandBufferSubmitInfo().setCommandBuffer(processInfo.commandBuffer->buffer(0));

        const auto submitInfo = vk::SubmitInfo2()
                                    .setPWaitSemaphoreInfos(waitInfos.data())
                                    .setWaitSemaphoreInfoCount(syncInfo.workWaitOnCount)
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
    }

    processInfo.commandBuffer->markSubmitted();

    // The Image manager owns the texture from here on: publish it into the pending record the caller registered.
    if (textureSlot.has_value())
    {
        textureSlot->set_value(
            std::expected<StarTextures::Texture, core::ResourceCreationError>{std::move(*finalTexture)});
        textureSlot.reset();
    }

    if (newTextureRequest->takesStagingOwnership)
        newTextureRequest->adoptStagingBuffer(*transferSrcBuffer);
    else
        processInfo.setInProcessDeps(std::move(transferSrcBuffer));
}

void TransferManagerThread::PublishFailure(InterThreadRequest &request, const core::ResourceCreationError &error)
{
    if (request.textureSlot.has_value())
    {
        request.textureSlot->set_value(std::unexpected(error));
        request.textureSlot.reset();
    }

    // Buffer requests publish nothing here: their consumer watches the completion flag and the record's resource.
    if (!request.textureTransferRequest)
        return;

    auto &texture = *request.textureTransferRequest;

    if (texture.stagingBufferSlot.has_value())
    {
        texture.stagingBufferSlot->set_value(std::unexpected(error));
        texture.stagingBufferSlot.reset();
    }

    texture.onTransferFailed(error.what());
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
