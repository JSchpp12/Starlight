#pragma once

#include "Enums.hpp"
#include "Handle.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"
#include "ManagerController_RenderResource_Texture.hpp"
#include "ManagerStorageContainer.hpp"
#include "StarBuffer.hpp"
#include "StarDevice.hpp"
#include "StarManager.hpp"
#include "TransferRequest_Memory.hpp"

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
        std::unique_ptr<ManagerController::RenderResource::Buffer> bufferRequest = nullptr;
        std::unique_ptr<StarBuffer> buffer = std::unique_ptr<StarBuffer>();
        std::unique_ptr<ManagerController::RenderResource::Texture> textureRequest = nullptr;
        std::unique_ptr<StarTexture> texture = std::unique_ptr<StarTexture>();

        FinalizedRenderRequest(std::unique_ptr<ManagerController::RenderResource::Buffer> bufferRequest)
            : bufferRequest(std::move(bufferRequest))
        {
        }

        FinalizedRenderRequest(std::unique_ptr<ManagerController::RenderResource::Texture> textureRequest)
            : textureRequest(std::move(textureRequest))
        {
        }
    };

    static void init(StarDevice &device, TransferWorker &worker, const int &totalNumFramesInFlight);

    static Handle addRequest(std::unique_ptr<ManagerController::RenderResource::Buffer> newRequest,
                             const bool &isHighPriority = false);

    static Handle addRequest(std::unique_ptr<ManagerController::RenderResource::Texture> newRequest,
                             const bool &isHighPriorirty = false);

    /// @brief Submit request to write new data to a buffer already created and associated to a handle
    /// @param newRequest New data request
    /// @param handle Handle to resource
    static void updateRequest(std::unique_ptr<ManagerController::RenderResource::Buffer> newRequest,
                              const Handle &handle, const bool &isHighPriority = false);

    static void update(const int &frameInFlightIndex);

    static bool isReady(const Handle &handle);

    static void waitForReady(const Handle &handle);

    static StarBuffer &getBuffer(const Handle &handle);

    static StarTexture &getTexture(const Handle &handle);

    static void destroy(const Handle &handle);

    static void cleanup(StarDevice &device);

  protected:
    static std::unique_ptr<ManagerStorageContainer<FinalizedRenderRequest>> bufferStorage;

    static std::set<boost::atomic<bool> *> highPriorityRequestCompleteFlags;
};
} // namespace star
