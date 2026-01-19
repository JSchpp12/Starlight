#pragma once

#include "Enums.hpp"
#include "ManagedHandleContainer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarManager.hpp"
#include "TransferRequest_Buffer.hpp"
#include "TransferRequest_Texture.hpp"
#include "device/StarDevice.hpp"

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <vector>

namespace star
{
class ManagerRenderResource : public StarManager
{
  private:
  public:
    template <typename T> struct FinalizedResourceRequest : public StarManager::FinalizedRequest
    {
        vk::Semaphore resourceSemaphore = VK_NULL_HANDLE;
        std::unique_ptr<T> resource = std::unique_ptr<T>();

        FinalizedResourceRequest() = default;
        FinalizedResourceRequest(const FinalizedResourceRequest &) = delete;
        FinalizedResourceRequest operator=(const FinalizedResourceRequest &) = delete;
        FinalizedResourceRequest(FinalizedResourceRequest &&other)
            : resourceSemaphore(std::move(other.resourceSemaphore)),
              resource(other.resource ? std::move(other.resource) : nullptr)
        {
        }
        FinalizedResourceRequest &operator=(FinalizedResourceRequest &&other)
        {
            if (this != &other)
            {
                resourceSemaphore = std::move(other.resourceSemaphore);
                if (other.resource)
                {
                    resource = std::move(other.resource);
                }
            }
            return *this;
        }

        explicit FinalizedResourceRequest(vk::Semaphore resourceSemaphore)
            : resourceSemaphore(std::move(resourceSemaphore))
        {
        }

        void cleanupRender(vk::Device &device)
        {
            if (resource)
            {
                resource->cleanupRender(device);
            }
        }
    };

    static void init(const Handle &deviceID, core::device::StarDevice *device,
                     std::shared_ptr<job::TransferWorker> worker, const int &totalNumFramesInFlight);

    static Handle addRequest(const Handle &deviceID, vk::Semaphore resourceSemaphore);

    static Handle addRequest(const Handle &deviceID, vk::Semaphore resourceSemaphore,
                             std::unique_ptr<TransferRequest::Buffer> newRequest,
                             vk::Semaphore *consumingQueueCompleteSemaphore = nullptr,
                             const bool &isHighPriority = false);

    static Handle addRequest(const Handle &deviceID, vk::Semaphore resourceSemaphore,
                             std::unique_ptr<TransferRequest::Texture> newRequest,
                             vk::Semaphore *consumingQueueCompleteSemaphore = nullptr,
                             const bool &isHighPriority = false);

    /// @brief Submit request to write new data to a buffer already created and associated to a handle
    /// @param newRequest New data request
    /// @param handle Handle to resource
    static void updateRequest(const Handle &deviceID, std::unique_ptr<TransferRequest::Buffer> newRequest,
                              const Handle &handle, const bool &isHighPriority = false);

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
        std::unique_ptr<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarTextures::Texture>, 1000>>,
        star::HandleHash>
        textureStorage;
    static std::unordered_map<
        Handle,
        std::unique_ptr<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarBuffers::Buffer>, 2000>>,
        star::HandleHash>
        bufferStorage;

    static std::unordered_map<Handle, std::set<boost::atomic<bool> *>, star::HandleHash>
        highPriorityRequestCompleteFlags;
};
} // namespace star
