#pragma once

#include "Enums.hpp"
#include "Handle.hpp"
#include "ManagedHandleContainer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarManager.hpp"
#include "TransferRequest_Buffer.hpp"
#include "TransferRequest_Texture.hpp"
#include "device/DeviceID.hpp"
#include "device/StarDevice.hpp"

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
  public:
    template <typename T> struct FinalizedResourceRequest : public StarManager::FinalizedRequest
    {
        vk::Semaphore resourceSemaphore = VK_NULL_HANDLE;
        std::unique_ptr<T> resource = std::unique_ptr<T>();

        FinalizedResourceRequest() = default;
        FinalizedResourceRequest(const FinalizedResourceRequest &) = delete;
        FinalizedResourceRequest operator=(const FinalizedResourceRequest &) = delete;
        FinalizedResourceRequest(FinalizedResourceRequest &&other)
            : resourceSemaphore(std::move(other.resourceSemaphore)), resource(std::move(other.resource))
        {
        }
        FinalizedResourceRequest &operator=(FinalizedResourceRequest &&other)
        {
            if (this != &other)
            {
                resourceSemaphore = std::move(other.resourceSemaphore);
                resource = std::move(other.resource);
            }
            return *this;
        }

        FinalizedResourceRequest(vk::Semaphore resourceSemaphore) : resourceSemaphore(std::move(resourceSemaphore))
        {
        }

        void cleanup(){
            resource.reset();
        }
    };

    static void init(core::device::DeviceID deviceID, std::shared_ptr<core::device::StarDevice> device,
                     std::shared_ptr<job::TransferWorker> worker, const int &totalNumFramesInFlight);

    static Handle addRequest(const core::device::DeviceID &deviceID, vk::Semaphore resourceSemaphore, const bool &isHighPriority = false);

    static Handle addRequest(const core::device::DeviceID &deviceID, vk::Semaphore resourceSemaphore,
                             std::unique_ptr<TransferRequest::Buffer> newRequest, const bool &isHighPriority = false);

    static Handle addRequest(const core::device::DeviceID &deviceID, vk::Semaphore resourceSemaphore,
                             std::unique_ptr<TransferRequest::Texture> newRequest, const bool &isHighPriorirty = false);

    /// @brief Submit request to write new data to a buffer already created and associated to a handle
    /// @param newRequest New data request
    /// @param handle Handle to resource
    static void updateRequest(const core::device::DeviceID &deviceID,
                              std::unique_ptr<TransferRequest::Buffer> newRequest, const Handle &handle,
                              const bool &isHighPriority = false);

    static void frameUpdate(const core::device::DeviceID &deviceID, const uint8_t &frameInFlightIndex);

    static bool isReady(const core::device::DeviceID &deviceID, const Handle &handle);

    static void waitForReady(const core::device::DeviceID &deviceID, const Handle &handle);

    static StarBuffers::Buffer &getBuffer(const core::device::DeviceID &deviceID, const Handle &handle);

    static StarTextures::Texture &getTexture(const core::device::DeviceID &deviceID, const Handle &handle);

    static void cleanup(const core::device::DeviceID &deviceID, core::device::StarDevice &device);

    template <typename T>
    static FinalizedResourceRequest<T> *get(const core::device::DeviceID &deviceID, const Handle &handle)
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
    static std::unordered_map<core::device::DeviceID, std::shared_ptr<core::device::StarDevice>> devices;
    static std::unordered_map<
        core::device::DeviceID,
        std::unique_ptr<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarTextures::Texture>,
                                              star::Handle_Type::texture, 50>>>
        textureStorage;
    static std::unordered_map<core::device::DeviceID,
                              std::unique_ptr<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarBuffers::Buffer>,
                                                                    star::Handle_Type::buffer, 100>>>
        bufferStorage;

    static std::unordered_map<core::device::DeviceID, std::set<boost::atomic<bool> *>> highPriorityRequestCompleteFlags;
};
} // namespace star
