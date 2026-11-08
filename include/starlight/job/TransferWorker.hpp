#pragma once

#include "Allocator.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarCommandBuffer.hpp"
#include "StarCommandPool.hpp"
#include "StarQueueFamily.hpp"
#include "StarTextures/Texture.hpp"
#include "TransferRequest_Buffer.hpp"
#include "TransferRequest_Texture.hpp"
#include "core/exceptions/ResourceCreationError.hpp"
#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "device/StarDevice.hpp"

#include <boost/atomic.hpp>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <queue>
#include <vector>

namespace star::job
{

class TransferManagerThread
{
  public:
    using TextureTransferPromise = std::promise<std::expected<StarTextures::Texture, core::ResourceCreationError>>;

    struct InterThreadRequest
    {
        core::graphics::GPUWorkSyncInfo workSyncInfo;
        boost::atomic<bool> *gpuDoneNotificationToMain = nullptr;
        std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest = nullptr;
        std::unique_ptr<TransferRequest::Texture> textureTransferRequest = nullptr;
        std::optional<TextureTransferPromise> textureSlot = std::nullopt;
        std::optional<std::unique_ptr<StarBuffers::Buffer> *> resultingBuffer = std::nullopt;

        InterThreadRequest() = default;

        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest,
                           std::unique_ptr<StarBuffers::Buffer> &resultingBuffer)
            : workSyncInfo(), gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              bufferTransferRequest(std::move(bufferTransferRequest)), resultingBuffer(&resultingBuffer)
        {
        }

        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest,
                           std::unique_ptr<StarBuffers::Buffer> &resultingBuffer,
                           core::graphics::GPUWorkSyncInfo workSyncInfo)
            : workSyncInfo(std::move(workSyncInfo)), gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              bufferTransferRequest(std::move(bufferTransferRequest)), resultingBuffer(&resultingBuffer)
        {
        }

        /// @brief Manager-owned texture: the worker publishes through the request's image-manager slot.
        /// @details No ManagerRenderResource record and no completion flag exist; the Pending future is the readiness
        /// signal, so gpuDoneNotificationToMain may be null.
        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Texture> textureTransferRequest,
                           std::optional<TextureTransferPromise> textureSlot)
            : workSyncInfo(), gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              textureTransferRequest(std::move(textureTransferRequest)), textureSlot(std::move(textureSlot))
        {
        }

        void reset()
        {
            gpuDoneNotificationToMain = nullptr;
            bufferTransferRequest = nullptr;
            textureTransferRequest = nullptr;
            textureSlot = std::nullopt;
            resultingBuffer = std::nullopt;
        }
    };

    class ProcessRequestInfo
    {
      public:
        std::shared_ptr<StarCommandPool> commandPool = nullptr;
        std::unique_ptr<StarCommandBuffer> commandBuffer = nullptr;

        ProcessRequestInfo(std::shared_ptr<StarCommandPool> commandPool,
                           std::unique_ptr<StarCommandBuffer> commandBuffer)
            : commandPool(std::move(commandPool)), commandBuffer(std::move(commandBuffer)) {};

        void setInProcessDeps(std::unique_ptr<StarBuffers::Buffer> nInProcessTransferSrcBuffer)
        {
            this->inProcessTransferSrcBuffer = std::move(nInProcessTransferSrcBuffer);
        }

        void markAsAvailable(vk::Device device)
        {
            if (inProcessTransferSrcBuffer)
            {
                inProcessTransferSrcBuffer->cleanupRender(device);
            }

            this->inProcessTransferSrcBuffer.reset();
            this->inProcessTransferSrcBuffer = nullptr;
        }

        bool isMarkedAsAvailable() const
        {
            return this->inProcessTransferSrcBuffer == nullptr;
        }

      private:
        std::unique_ptr<StarBuffers::Buffer> inProcessTransferSrcBuffer = nullptr;
    };

    static void CreateBuffer(core::device::StarDevice &device, StarQueue &queue,
                             const vk::PhysicalDeviceProperties &deviceProperties,
                             const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                             ProcessRequestInfo &processInfo, TransferRequest::Buffer *newBufferRequest,
                             std::unique_ptr<StarBuffers::Buffer> *resultingBuffer,
                             boost::atomic<bool> *gpuDoneSignalMain, core::graphics::GPUWorkSyncInfo &syncInfo);

    static void CreateTexture(core::device::StarDevice &device, StarQueue &queue,
                              const vk::PhysicalDeviceProperties &deviceProperties,
                              const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                              ProcessRequestInfo &processInfo, TransferRequest::Texture *newTextureRequest,
                              std::optional<TextureTransferPromise> &textureSlot,
                              boost::atomic<bool> *gpuDoneSignalToMain, core::graphics::GPUWorkSyncInfo &syncInfo);

    static void CheckForCleanups(vk::Device device, std::queue<std::unique_ptr<ProcessRequestInfo>> &processingInfos);

    /// @brief Publish a terminal transfer failure into any manager slot the request owns.
    /// @details Called on the worker's outermost failure boundary so consumers receive a blank resource plus the
    /// error that caused the failure instead of a broken promise.
    static void PublishFailure(InterThreadRequest &request, const core::ResourceCreationError &error);
};

} // namespace star::job
