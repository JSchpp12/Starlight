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

#include <boost/atomic.hpp>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <queue>
#include <variant>
#include <vector>

namespace star::core::device
{
class StarDevice;
}
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

    /// @brief One transfer submission, aggregate-initialized by the submitter.
    struct InterThreadRequest
    {
        TransferData data;
        std::optional<core::graphics::WorkWaitInfo> workWaitOn;
        core::graphics::SemaphoreSyncPoint workSignalWhenDone;
        boost::atomic<bool> *gpuDoneNotificationToMain = nullptr;

        void reset()
        {
            workWaitOn.reset();
            workSignalWhenDone = {};
            gpuDoneNotificationToMain = nullptr;
            data = BufferTransferData{};
        }
    };

    class ProcessRequestInfo
    {
      public:
        std::shared_ptr<StarCommandPool> commandPool = nullptr;
        std::unique_ptr<StarCommandBuffer> commandBuffer = nullptr;
        /// @brief Whether the current recording reached the queue. A failed submission never signals its allocated
        /// timeline value, so the worker host-signals it instead.
        bool submitted = false;

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
                             boost::atomic<bool> *gpuDoneSignalMain,
                             const std::optional<core::graphics::WorkWaitInfo> &workWaitOn,
                             const core::graphics::SemaphoreSyncPoint &workSignalWhenDone);

    /// @brief Create the texture described by a texture transfer request and publish it into its slot.
    static void CreateTexture(core::device::StarDevice &device, StarQueue &queue,
                              const vk::PhysicalDeviceProperties &deviceProperties,
                              const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                              ProcessRequestInfo &processInfo, TextureTransferData &requestData,
                              boost::atomic<bool> *gpuDoneSignalToMain,
                              const std::optional<core::graphics::WorkWaitInfo> &workWaitOn,
                              const core::graphics::SemaphoreSyncPoint &workSignalWhenDone);

    static void CheckForCleanups(vk::Device device, std::queue<std::unique_ptr<ProcessRequestInfo>> &processingInfos);

    /// @brief Signal the timeline value allocated for a submission that never reached the queue.
    /// @details Waits for the worker queue to drain so the host signal cannot jump ahead of pending device signals,
    /// then signals the value. Without this, later transfers chained on that value would block forever.
    static void SignalFailedSubmission(core::device::StarDevice &device, StarQueue &queue,
                                       const core::graphics::SemaphoreSyncPoint &signalInfo);

    /// @brief Publish a terminal transfer failure into any manager slot the request owns.
    /// @details Called on the worker's outermost failure boundary so consumers receive a blank resource plus the
    /// error that caused the failure instead of a broken promise.
    static void PublishFailure(InterThreadRequest &request, const core::ResourceCreationError &error);
};

} // namespace star::job
