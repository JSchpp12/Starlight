#pragma once

#include "Enums.hpp"
#include "Handle.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"
#include "ManagerController_RenderResource_Texture.hpp"
#include "ManagerStorageContainer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarManager.hpp"
#include "TransferRequest_Memory.hpp"
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
    struct FinalizedRenderRequest : public StarManager::FinalizedRequest
    {
        vk::Semaphore resourceSemaphore;
        std::unique_ptr<ManagerController::RenderResource::Buffer> bufferRequest = nullptr;
        std::unique_ptr<StarBuffers::Buffer> buffer = std::unique_ptr<StarBuffers::Buffer>();
        std::unique_ptr<ManagerController::RenderResource::Texture> textureRequest = nullptr;
        std::unique_ptr<StarTextures::Texture> texture = std::unique_ptr<StarTextures::Texture>();

        FinalizedRenderRequest(vk::Semaphore resourceSemaphore,
                               std::unique_ptr<ManagerController::RenderResource::Buffer> bufferRequest)
            : resourceSemaphore(std::move(resourceSemaphore)), bufferRequest(std::move(bufferRequest))
        {
        }

        FinalizedRenderRequest(vk::Semaphore resourceSemaphore,
                               std::unique_ptr<ManagerController::RenderResource::Texture> textureRequest)
            : resourceSemaphore(std::move(resourceSemaphore)), textureRequest(std::move(textureRequest))
        {
        }
    };

    static void init(core::device::DeviceID deviceID, std::shared_ptr<core::device::StarDevice> device,
                     std::shared_ptr<job::TransferWorker> worker, const int &totalNumFramesInFlight);

    static Handle addRequest(const core::device::DeviceID &deviceID, vk::Semaphore resourceSemaphore,
                             std::unique_ptr<ManagerController::RenderResource::Buffer> newRequest,
                             const bool &isHighPriority = false);

    static Handle addRequest(const core::device::DeviceID &deviceID, vk::Semaphore resourceSemaphore,
                             std::unique_ptr<ManagerController::RenderResource::Texture> newRequest,
                             const bool &isHighPriorirty = false);

    /// @brief Submit request to write new data to a buffer already created and associated to a handle
    /// @param newRequest New data request
    /// @param handle Handle to resource
    static void updateRequest(const core::device::DeviceID &deviceID,
                              std::unique_ptr<ManagerController::RenderResource::Buffer> newRequest,
                              const Handle &handle, const bool &isHighPriority = false);

    static void update(const core::device::DeviceID &deviceID, const int &frameInFlightIndex);

    static bool isReady(const core::device::DeviceID &deviceID, const Handle &handle);

    static void waitForReady(const core::device::DeviceID &deviceID, const Handle &handle);

    static StarBuffers::Buffer &getBuffer(const core::device::DeviceID &deviceID, const Handle &handle);

    static StarTextures::Texture &getTexture(const core::device::DeviceID &deviceID, const Handle &handle);

    static void destroy(const core::device::DeviceID &deviceID, const Handle &handle);

    static void cleanup(const core::device::DeviceID &deviceID, core::device::StarDevice &device);

    static FinalizedRenderRequest& get(const core::device::DeviceID &deviceID, const Handle &handle); 

  protected:
    static std::unordered_map<core::device::DeviceID, std::shared_ptr<core::device::StarDevice>> devices;
    static std::unordered_map<core::device::DeviceID, std::unique_ptr<ManagerStorageContainer<FinalizedRenderRequest>>>
        bufferStorage;

    static std::unordered_map<core::device::DeviceID, std::set<boost::atomic<bool> *>> highPriorityRequestCompleteFlags;
};
} // namespace star
