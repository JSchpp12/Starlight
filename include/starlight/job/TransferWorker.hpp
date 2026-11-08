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
#include <queue>
#include <variant>
#include <vector>

namespace star::job
{

class TransferManagerThread
{
  public:
    using BufferTransferPromise = std::promise<std::expected<StarBuffers::Buffer, core::ResourceCreationError>>;
    using TextureTransferPromise = std::promise<std::expected<StarTextures::Texture, core::ResourceCreationError>>;

    struct BufferTransferData
    {
        std::unique_ptr<TransferRequest::Buffer> request;
        /// Caller-owned destination (legacy ManagerRenderResource path) or a buffer-manager slot.
        std::variant<std::unique_ptr<StarBuffers::Buffer> *, BufferTransferPromise> destination;
    };

    struct TextureTransferData
    {
        std::unique_ptr<TransferRequest::Texture> request;
        TextureTransferPromise slot;
    };

    /// @brief One of the two transfer paths a request can describe.
    using TransferData = std::variant<BufferTransferData, TextureTransferData>;

    struct InterThreadRequest
    {
        core::graphics::GPUWorkSyncInfo workSyncInfo;
        boost::atomic<bool> *gpuDoneNotificationToMain = nullptr;
        TransferData data;

        InterThreadRequest() = default;

        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest,
                           std::unique_ptr<StarBuffers::Buffer> &resultingBuffer)
            : workSyncInfo(), gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              data(BufferTransferData{std::move(bufferTransferRequest), &resultingBuffer})
        {
        }

        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest,
                           std::unique_ptr<StarBuffers::Buffer> &resultingBuffer,
                           core::graphics::GPUWorkSyncInfo workSyncInfo)
            : workSyncInfo(std::move(workSyncInfo)), gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              data(BufferTransferData{std::move(bufferTransferRequest), &resultingBuffer})
        {
        }

        /// @brief Manager-owned buffer: the worker publishes through the buffer-manager slot.
        /// @details No ManagerRenderResource record and no completion flag exist; the Pending future is the readiness
        /// signal, so gpuDoneNotificationToMain may be null.
        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest,
                           BufferTransferPromise bufferSlot)
            : workSyncInfo(), gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              data(BufferTransferData{std::move(bufferTransferRequest), std::move(bufferSlot)})
        {
        }

        /// @brief Manager-owned texture: the worker publishes through the request's image-manager slot.
        /// @details No ManagerRenderResource record and no completion flag exist; the Pending future is the readiness
        /// signal, so gpuDoneNotificationToMain may be null.
        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Texture> textureTransferRequest,
                           TextureTransferPromise textureSlot)
            : workSyncInfo(), gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              data(TextureTransferData{std::move(textureTransferRequest), std::move(textureSlot)})
        {
        }

        void reset()
        {
            gpuDoneNotificationToMain = nullptr;
            workSyncInfo = core::graphics::GPUWorkSyncInfo{};
            data = BufferTransferData{};
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

    /// @brief Create the buffer described by a buffer transfer request and publish it to its destination.
    static void CreateBuffer(core::device::StarDevice &device, StarQueue &queue,
                             const vk::PhysicalDeviceProperties &deviceProperties,
                             const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                             ProcessRequestInfo &processInfo, BufferTransferData &requestData,
                             boost::atomic<bool> *gpuDoneSignalMain, core::graphics::GPUWorkSyncInfo &syncInfo);

    /// @brief Create the texture described by a texture transfer request and publish it into its slot.
    static void CreateTexture(core::device::StarDevice &device, StarQueue &queue,
                              const vk::PhysicalDeviceProperties &deviceProperties,
                              const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                              ProcessRequestInfo &processInfo, TextureTransferData &requestData,
                              boost::atomic<bool> *gpuDoneSignalToMain, core::graphics::GPUWorkSyncInfo &syncInfo);

    static void CheckForCleanups(vk::Device device, std::queue<std::unique_ptr<ProcessRequestInfo>> &processingInfos);

    /// @brief Publish a terminal transfer failure into any manager slot the request owns.
    /// @details Called on the worker's outermost failure boundary so consumers receive a blank resource plus the
    /// error that caused the failure instead of a broken promise.
    static void PublishFailure(InterThreadRequest &request, const core::ResourceCreationError &error);
};

} // namespace star::job
