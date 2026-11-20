#pragma once

#include "Enums.hpp"
#include "ManagedHandleContainer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "TransferRequest_Buffer.hpp"
#include "TransferRequest_Texture.hpp"
#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "job/TaskManager.hpp"
#include "job/tasks/TransferTask.hpp"
#include "starlight/core/CommandBus.hpp"

#include <boost/atomic.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <stack>
#include <star_common/Handle.hpp>
#include <vulkan/vulkan.hpp>

namespace star::core::device
{
class StarDevice;
}

namespace star
{
class ManagerRenderResource
{
  private:
  public:
    struct FinalizedRequest
    {
        boost::atomic<bool> cpuWorkDoneByTransferThread = true;

        FinalizedRequest()
        {
        }
    };

    /// @brief A submitted upload's signal: the raw descriptor used to wait on it, plus the semaphore-manager identity
    /// of its timeline.
    struct UploadSignal
    {
        core::graphics::SemaphoreSyncPoint signal;
        /// Semaphore-manager record of the timeline `signal` belongs to.
        Handle record;
    };

    template <typename T> struct FinalizedResourceRequest : public FinalizedRequest
    {
        std::unique_ptr<T> resource = std::unique_ptr<T>();
        std::optional<UploadSignal> gpuWorkDoneSignal;

        FinalizedResourceRequest() = default;
        FinalizedResourceRequest(const FinalizedResourceRequest &) = delete;
        FinalizedResourceRequest operator=(const FinalizedResourceRequest &) = delete;
        FinalizedResourceRequest(FinalizedResourceRequest &&other) noexcept : resource(std::move(other.resource))
        {
            cpuWorkDoneByTransferThread.store(other.cpuWorkDoneByTransferThread.load());
            gpuWorkDoneSignal = other.gpuWorkDoneSignal;
        }
        FinalizedResourceRequest &operator=(FinalizedResourceRequest &&other) noexcept
        {
            if (this != &other)
            {
                resource = std::move(other.resource);
                cpuWorkDoneByTransferThread.store(other.cpuWorkDoneByTransferThread.load());
                gpuWorkDoneSignal = other.gpuWorkDoneSignal;
            }
            return *this;
        }

        void cleanupRender(vk::Device &device)
        {
            if (resource)
                resource->cleanupRender(device);
        }
    };

    static void init(const Handle &deviceID, core::device::StarDevice *device, star::core::CommandBus &cmdBus);

    static Handle addRequest(const Handle &deviceID);

    static Handle addRequest(const Handle &deviceID, std::unique_ptr<TransferRequest::Buffer> newRequest,
                             vk::Semaphore *consumingQueueCompleteSemaphore = nullptr,
                             uint32_t *outTransferQueueFamilyIndex = nullptr);

    /// @brief Submit request to write new data to a buffer already created and associated to a handle
    /// @param newRequest New data request
    /// @param waitInfo GPU synchronization info which transfer worker will wait on before submitting its commands to
    /// the gpu
    /// @param handle Handle to resource
    static void updateRequest(const Handle &deviceID, std::unique_ptr<TransferRequest::Buffer> newRequest,
                              const Handle &handle,
                              std::optional<core::graphics::SemaphoreSyncPoint> waitInfo = std::nullopt,
                              uint32_t *outTransferQueueFamilyIndex = nullptr);

    static bool isReady(const Handle &deviceID, const Handle &handle);

    /// @brief True while the transfer worker has not yet published the request's result.
    static bool isPending(const Handle &deviceID, const Handle &handle);

    static void waitForReady(const Handle &deviceID, const Handle &handle);

    static StarBuffers::Buffer &getBuffer(const Handle &deviceID, const Handle &handle);

    static void cleanup(const Handle &deviceID, core::device::StarDevice &device);

    template <typename T> static FinalizedResourceRequest<T> *get(const Handle &deviceID, const Handle &handle)
    {
        if constexpr (std::is_same_v<T, StarBuffers::Buffer>)
        {
            return &bufferStorage.at(deviceID)->get(handle);
        }
        else
        {
            // todo: better error handling later
            throw std::runtime_error("Invalid type provided");
        }
        return nullptr;
    }

  protected:
    static std::unordered_map<Handle, core::device::StarDevice *, star::HandleHash> devices;
    static std::unordered_map<
        Handle, std::unique_ptr<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarBuffers::Buffer>>>,
        star::HandleHash>
        bufferStorage;

    static star::core::CommandBus *s_cmdBus;
};

} // namespace star
