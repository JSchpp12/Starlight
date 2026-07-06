#pragma once

#include "Enums.hpp"
#include "ManagedHandleContainer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "TransferRequest_Buffer.hpp"
#include "TransferRequest_Texture.hpp"
#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "device/StarDevice.hpp"
#include "job/TaskManager.hpp"
#include "job/tasks/TransferTask.hpp"
#include "starlight/core/CommandBus.hpp"
#include "starlight/wrappers/graphics/StarSemaphore.hpp"

#include <star_common/Handle.hpp>

#include <boost/atomic.hpp>
#include <vulkan/vulkan.hpp>

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <vector>

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

    template <typename T> struct FinalizedResourceRequest : public FinalizedRequest
    {
        std::unique_ptr<T> resource = std::unique_ptr<T>();
        star::StarSemaphore gpuWorkDoneSignaledInfo; 

        FinalizedResourceRequest() = default;
        FinalizedResourceRequest(const FinalizedResourceRequest &) = delete;
        FinalizedResourceRequest operator=(const FinalizedResourceRequest &) = delete;
        FinalizedResourceRequest(FinalizedResourceRequest &&other) noexcept
            : resource(other.resource ? std::move(other.resource) : nullptr)
        {
        }
        FinalizedResourceRequest &operator=(FinalizedResourceRequest &&other) noexcept
        {
            if (this != &other)
            {
                if (other.resource)
                {
                    resource = std::move(other.resource);
                }
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
                             const bool &isHighPriority = false, uint32_t *outTransferQueueFamilyIndex = nullptr);

    static Handle addRequest(const Handle &deviceID, std::unique_ptr<TransferRequest::Texture> newRequest,
                             vk::Semaphore *consumingQueueCompleteSemaphore = nullptr,
                             const bool &isHighPriority = false, uint32_t *outTransferQueueFamilyIndex = nullptr);

    /// @brief Submit request to write new data to a buffer already created and associated to a handle
    /// @param newRequest New data request
    /// @param waitInfo GPU synchronization info which transfer worker will wait on before submitting its commands to
    /// the gpu
    /// @param handle Handle to resource
    static void updateRequest(const Handle &deviceID, std::unique_ptr<TransferRequest::Buffer> newRequest,
                              const Handle &handle,
                              std::optional<core::graphics::SemaphoreInfo> waitInfo = std::nullopt,
                              const bool &isHighPriority = false, uint32_t *outTransferQueueFamilyIndex = nullptr);

    static void frameUpdate(const Handle &deviceID, const uint8_t &frameInFlightIndex);

    static bool isReady(const Handle &deviceID, const Handle &handle);

    static void waitForReady(const Handle &deviceID, const Handle &handle);

    static StarBuffers::Buffer &getBuffer(const Handle &deviceID, const Handle &handle);

    static StarTextures::Texture &getTexture(const Handle &deviceID, const Handle &handle);

    static void cleanup(const Handle &deviceID, core::device::StarDevice &device);

    template <typename T> static FinalizedResourceRequest<T> *get(const Handle &deviceID, const Handle &handle)
    {
        if constexpr (std::is_same_v<T, StarTextures::Texture>)
        {
            return &textureStorage.at(deviceID)->get(handle);
        }
        else if constexpr (std::is_same_v<T, StarBuffers::Buffer>)
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
        Handle,
        std::unique_ptr<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarTextures::Texture>, 2000>>,
        star::HandleHash>
        textureStorage;
    static std::unordered_map<
        Handle,
        std::unique_ptr<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarBuffers::Buffer>, 3500>>,
        star::HandleHash>
        bufferStorage;

    static std::unordered_map<Handle, std::set<boost::atomic<bool> *>, star::HandleHash>
        highPriorityRequestCompleteFlags;

    static star::core::CommandBus *s_cmdBus;
};

} // namespace star
