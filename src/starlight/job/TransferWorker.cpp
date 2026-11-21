#include "job/TransferWorker.hpp"

#include "core/Exceptions.hpp"

#include "device/StarDevice.hpp"
#include "logging/LoggingFactory.hpp"

#include <sstream>
#include <type_traits>
#include <variant>

namespace star::job
{

void TransferManagerThread::CreateBuffer(core::device::StarDevice &device, StarQueue &queue,
                                         const vk::PhysicalDeviceProperties &deviceProperties,
                                         const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                                         ProcessRequestInfo &processInfo, BufferTransferData &requestData,
                                         boost::atomic<bool> *gpuDoneSignalMain,
                                         const std::optional<core::graphics::WorkWaitInfo> &workWaitOn,
                                         const core::graphics::SemaphoreSyncPoint &workSignalWhenDone)
{
    auto &newBufferRequest = *requestData.request;

    auto transferSrcBuffer = newBufferRequest.createStagingBuffer(device);
    if (transferSrcBuffer->getBufferSize() == 0)
        STAR_THROW("Failed to create transfer src buffer");

    auto newResult = newBufferRequest.createFinal(device, allTransferQueueFamilyIndicesInUse);
    if (newResult->getBufferSize() == 0)
    {
        STAR_THROW("Failed to create final buffer");
    }

    auto *resultingBuffer = std::get_if<std::unique_ptr<StarBuffers::Buffer> *>(&requestData.destination);
    StarBuffers::Buffer *destination = nullptr;
    if (resultingBuffer != nullptr)
    {
        if (!**resultingBuffer ||
            ((*resultingBuffer)->get() && newResult->getBufferSize() > (*resultingBuffer)->get()->getBufferSize()))
        {
            (*resultingBuffer)->swap(newResult);
        }
        destination = (*resultingBuffer)->get();
    }
    else
    {
        destination = newResult.get();
    }

    newBufferRequest.writeDataToStageBuffer(*transferSrcBuffer);

    newBufferRequest.copyFromTransferSRCToDST(*transferSrcBuffer, *destination, processInfo.commandBuffer->buffer(0));

    processInfo.commandBuffer->buffer(0).end();

    {
        const auto signalInfo = vk::SemaphoreSubmitInfo()
                                    .setSemaphore(workSignalWhenDone.semaphore)
                                    .setValue(workSignalWhenDone.signalValue)
                                    .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

        const uint8_t waitCount = workWaitOn.has_value() ? workWaitOn->count : 0;
        std::array<vk::SemaphoreSubmitInfo, 2> waitInfos{};
        for (uint8_t i = 0; i < waitCount; ++i)
        {
            const auto &wait = workWaitOn->waits[i];
            waitInfos[i].setSemaphore(wait.semaphore).setValue(wait.signalValue).setStageMask(wait.where);
        }

        const auto cbInfo = vk::CommandBufferSubmitInfo().setCommandBuffer(processInfo.commandBuffer->buffer(0));

        const auto submitInfo = vk::SubmitInfo2()
                                    .setPWaitSemaphoreInfos(waitInfos.data())
                                    .setWaitSemaphoreInfoCount(waitCount)
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

    processInfo.submitted = true;

    // The Buffer manager owns the buffer from here on: publish it into the pending record the caller registered.
    if (auto *bufferSlot = std::get_if<BufferTransferPromise>(&requestData.destination))
    {
        bufferSlot->set_value(std::expected<StarBuffers::Buffer, core::ResourceCreationError>{std::move(*newResult)});
    }

    // The request may publish its one-shot contribution after submission and slot publication.
    newBufferRequest.onTransferComplete();

    processInfo.setInProcessDeps(std::move(transferSrcBuffer));
}

void TransferManagerThread::CreateTexture(core::device::StarDevice &device, StarQueue &queue,
                                          const vk::PhysicalDeviceProperties &deviceProperties,
                                          const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                                          ProcessRequestInfo &processInfo, TextureTransferData &requestData,
                                          boost::atomic<bool> *gpuDoneSignalToMain,
                                          const std::optional<core::graphics::WorkWaitInfo> &workWaitOn,
                                          const core::graphics::SemaphoreSyncPoint &workSignalWhenDone)
{
    auto &newTextureRequest = *requestData.request;

    std::unique_ptr<StarBuffers::Buffer> transferSrcBuffer = newTextureRequest.createStagingBuffer(device);
    if (transferSrcBuffer && transferSrcBuffer->getBufferSize() == 0)
        STAR_THROW("Failed to create transfer src buffer");

    bool newImageCreated = true;

    auto finalTexture = newTextureRequest.createFinal(device, allTransferQueueFamilyIndicesInUse);

    if (transferSrcBuffer)
    {
        newTextureRequest.writeDataToStageBuffer(*transferSrcBuffer);

        if (!newImageCreated)
        {
            STAR_THROW("Unsupported image operation in transfer manager");
        }

        newTextureRequest.copyFromTransferSRCToDST(*transferSrcBuffer, *finalTexture,
                                                   processInfo.commandBuffer->buffer(0));
    }

    processInfo.commandBuffer->buffer(0).end();

    {
        const auto signalInfo = vk::SemaphoreSubmitInfo()
                                    .setSemaphore(workSignalWhenDone.semaphore)
                                    .setValue(workSignalWhenDone.signalValue)
                                    .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

        const uint8_t waitCount = workWaitOn.has_value() ? workWaitOn->count : 0;
        std::array<vk::SemaphoreSubmitInfo, 2> waitInfos{};
        for (uint8_t i = 0; i < waitCount; ++i)
        {
            const auto &wait = workWaitOn->waits[i];
            waitInfos[i].setSemaphore(wait.semaphore).setValue(wait.signalValue).setStageMask(wait.where);
        }

        const auto cbInfo = vk::CommandBufferSubmitInfo().setCommandBuffer(processInfo.commandBuffer->buffer(0));

        const auto submitInfo = vk::SubmitInfo2()
                                    .setPWaitSemaphoreInfos(waitInfos.data())
                                    .setWaitSemaphoreInfoCount(waitCount)
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

    processInfo.submitted = true;

    // The Image manager owns the texture from here on: publish it into the pending record the caller registered.
    requestData.slot.set_value(
        std::expected<StarTextures::Texture, core::ResourceCreationError>{std::move(*finalTexture)});

    // The request may publish its one-shot contribution after submission.
    newTextureRequest.onTransferComplete();

    // Staging, when used, is worker-owned and fence-freed; no caller adopts it.
    if (transferSrcBuffer)
        processInfo.setInProcessDeps(std::move(transferSrcBuffer));
}

void TransferManagerThread::SignalFailedSubmission(core::device::StarDevice &device, StarQueue &queue,
                                                   const core::graphics::SemaphoreSyncPoint &signalInfo)
{
    if (signalInfo.semaphore == VK_NULL_HANDLE || signalInfo.signalValue == 0)
        return;

    core::logging::warning("Transfer submission failed; signaling timeline value ", signalInfo.signalValue,
                           " so chained transfers are not blocked");

    try
    {
        // Host signals are not ordered against device submissions. Drain this worker's queue so every signal already
        // submitted to the timeline has been applied before advancing past the failed value.
        queue.getVulkanQueue().waitIdle();

        device.getVulkanDevice().signalSemaphore(
            vk::SemaphoreSignalInfo().setSemaphore(signalInfo.semaphore).setValue(signalInfo.signalValue));
    }
    catch (const std::exception &e)
    {
        std::ostringstream oss;
        oss << "Failed to signal timeline value " << signalInfo.signalValue
            << " after a transfer failure; chained transfers will block: " << e.what();
        STAR_THROW(oss.str());
    }
}

void TransferManagerThread::PublishFailure(InterThreadRequest &request, const core::ResourceCreationError &error)
{
    std::visit(
        [&error](auto &data) {
            using DataType = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<DataType, BufferTransferData>)
            {
                // Manager-owned buffers receive the failure reason through their slot.
                if (auto *bufferSlot = std::get_if<BufferTransferPromise>(&data.destination))
                {
                    bufferSlot->set_value(std::unexpected(error));
                }

                // The request owns any extra promises (e.g. skybox conversion inputs); reject them too.
                if (data.request)
                    data.request->onTransferFailed(error.what());
            }
            else
            {
                data.slot.set_value(std::unexpected(error));

                auto &texture = *data.request;
                texture.onTransferFailed(error.what());
            }
        },
        request.data);
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
